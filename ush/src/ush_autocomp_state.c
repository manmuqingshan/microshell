#include "ush_internal.h"
#include "ush_config.h"
#include "ush_utils.h"
#include "ush_node.h"

#include <string.h>

#if USH_CONFIG_ENABLE_FEATURE_AUTOCOMPLETE == 1

void ush_autocomp_state_recall_suffix(struct ush_object *self)
{
        char *suffix = self->autocomp_input + strlen(self->autocomp_input) - self->autocomp_suffix_len;
        if (strlen(suffix) > 0) {
                ush_write_pointer(self, suffix, USH_STATE_READ_CHAR);
        } else {
                ush_autocomp_prepare_candidates(self);
                ush_write_pointer(self, "\r\n", USH_STATE_AUTOCOMP_CANDIDATES_PRINT);
        }
}

void ush_autocomp_state_prepare(struct ush_object *self)
{
        self->autocomp_input = ush_utils_get_last_arg(self->desc->input_buffer);
        if (self->autocomp_input == NULL) {
                self->state = USH_STATE_AUTOCOMP_PROMPT_PREPARE;
        } else {
                self->state = USH_STATE_AUTOCOMP_CANDIDATES_START;
        }
}

void ush_autocomp_state_candidates_start(struct ush_object *self)
{
        self->autocomp_prev_count = 0;
        self->autocomp_suffix_len = 0;
        ush_autocomp_prepare_candidates(self);
        self->state = USH_STATE_AUTOCOMP_CANDIDATES_COUNT;
}

void ush_autocomp_state_candidates_process(struct ush_object *self)
{
        if (self->process_node == NULL) {
                char abs_path[USH_CONFIG_PATH_MAX_LENGTH];
                if (self->process_stage == 0) {
                        self->process_node = self->current_node;
                        self->process_index = 0;
                        self->process_index_item = 0;
                        self->process_stage = 1;
                } else if (self->process_stage == 1) {
                        ush_node_get_absolute_path(self, self->autocomp_input, abs_path);
                        if (self->autocomp_input[0] == '\0') {
                                self->process_node = self->current_node;
                        } else {
                                self->process_node = ush_node_get_parent_by_path(self, abs_path);
                                if (self->process_node == NULL)
                                        self->process_node = self->current_node;
                        }

                        self->process_node = self->process_node->childs;
                        self->process_index = 0;
                        self->process_index_item = 0;
                        self->process_stage = 2;
                } else if (self->process_stage == 2) {
                        self->autocomp_prev_state = self->state;
                        self->state = USH_STATE_AUTOCOMP_CANDIDATES_FINISH;
                } else {
                        USH_ASSERT(false);
                }
                return;                        
        }

        if (self->process_stage == 0) {
                if (self->process_index_item >= self->process_node->file_list_size) {
                        self->process_index_item = 0;
                        self->process_node = self->process_node->next;
                        return;
                }
        } else if (self->process_stage == 1) {
                if (self->process_index_item >= self->process_node->file_list_size) {
                        self->process_index_item = 0;
                        self->process_node = NULL;
                        return;
                }
        } else if (self->process_stage == 2) {
                /* do nothing */
        } else {
                USH_ASSERT(false);
        }
        
        struct ush_file_descriptor const *file = NULL;
        
        if ((self->process_stage == 0) || (self->process_stage == 1)) {
                file = &self->process_node->file_list[self->process_index_item];
                if (ush_utils_startswith((char*)file->name, self->autocomp_input) == false) {
                        self->process_index_item++;
                        self->process_index = 0;
                        return;
                }
        } else if (self->process_stage == 2) {
                ush_utils_path_last(self->process_node->path, &self->autocomp_name);
                if (ush_utils_startswith(self->autocomp_name, self->autocomp_input) == false) {
                        self->process_node = self->process_node->next;
                        self->process_index = 0;
                        return;
                }
        } else {
                USH_ASSERT(false);
        }

        switch (self->process_index) {
        case 0:
                if ((self->process_stage == 0) || (self->process_stage == 1)) {
                        if (self->state == USH_STATE_AUTOCOMP_CANDIDATES_PRINT)
                                ush_write_pointer(self, (char*)file->name, self->state);
                        self->process_index = 1;
                        self->autocomp_count++;
                        self->autocomp_candidate_name = (char*)file->name;
                } else if (self->process_stage == 2) {
                        if (self->state == USH_STATE_AUTOCOMP_CANDIDATES_PRINT)
                                ush_write_pointer(self, self->autocomp_name, self->state);
                        self->process_index = 1;
                        self->autocomp_count++;
                        self->autocomp_candidate_name = self->autocomp_name;
                } else {
                        USH_ASSERT(false);
                }               
                break;
        case 1:
                if (self->state == USH_STATE_AUTOCOMP_CANDIDATES_PRINT)
                        ush_write_pointer(self, "\r\n", self->state);
                self->process_index = 2;
                break;
        case 2:
                if ((self->process_stage == 0) || (self->process_stage == 1)) {
                        self->process_index_item++;
                        self->process_index = 0;
                } else if (self->process_stage == 2) {
                        self->process_node = self->process_node->next;
                        self->process_index = 0;
                } else {
                        USH_ASSERT(false);
                }                        
                break;
        default:
                USH_ASSERT(false);
                break;
        }
}

void ush_autocomp_state_candidates_finish(struct ush_object *self)
{
        if (self->autocomp_prev_state == USH_STATE_AUTOCOMP_CANDIDATES_PRINT) {
                self->state = USH_STATE_AUTOCOMP_PROMPT;
                return;
        }

        if (self->autocomp_prev_state == USH_STATE_AUTOCOMP_CANDIDATES_COUNT) {
                switch (self->autocomp_count) {
                case 0:
                        self->state = USH_STATE_READ_CHAR;
                        break;
                case 1: {
                        char *suffix = self->autocomp_input + strlen(self->autocomp_input);
                        strcpy(self->autocomp_input, self->autocomp_candidate_name);
                        self->in_pos = strlen(self->desc->input_buffer);
                        ush_write_pointer(self, suffix, USH_STATE_READ_CHAR);
                        break;
                }
                default:
                        self->autocomp_suffix_len = 0;
                        ush_autocomp_optimize_continue(self);
                        break;
                }

        } else if (self->autocomp_prev_state == USH_STATE_AUTOCOMP_CANDIDATES_OPTIMISE) {
                if (self->autocomp_count < self->autocomp_prev_count) {
                        if (self->in_pos > 0)
                                self->in_pos--;
                        self->desc->input_buffer[self->in_pos] = '\0';
                        self->autocomp_suffix_len--;

                        self->state = USH_STATE_AUTOCOMP_RECALL_SUFFIX;
                        return;
                }

                ush_autocomp_optimize_continue(self);
        } else {
                USH_ASSERT(false);
        }
}

#endif /* USH_CONFIG_ENABLE_FEATURE_AUTOCOMPLETE */