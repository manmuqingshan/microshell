#include "ush.h"
#include "ush_internal.h"

#include <string.h>

void ush_buildin_cmd_xxd_callback(struct ush_object *self, struct ush_file_descriptor const *file, int argc, char *argv[])
{
        (void)file;

        if (argc != 2) {
                ush_print_status(self, USH_STATUS_ERROR_COMMAND_WRONG_ARGUMENTS);
                return;
        }

        struct ush_file_descriptor const *f = ush_file_find_by_name(self, argv[1]);
        if (f == NULL) {
                ush_print_status(self, USH_STATUS_ERROR_FILE_NOT_EXISTS);
                return;
        }

        if (f->get_data == NULL) {
                ush_print_status(self, USH_STATUS_ERROR_FILE_NO_DATA);
                return;
        }

        self->process_data_size = f->get_data(self, f, &self->process_data);
        ush_process_start(self, file);
}

bool ush_buildin_cmd_xxd_service(struct ush_object *self, struct ush_file_descriptor const *file)
{
        (void)file;
        
        USH_ASSERT(self != NULL);
        USH_ASSERT(file != NULL);

        bool processed = true;

        switch (self->state) {

        case USH_STATE_PROCESS_START:
                self->process_index = 0;
                self->process_index_item = 0;
                self->state = USH_STATE_PROCESS_SERVICE;
                break;

        case USH_STATE_PROCESS_SERVICE: {
                switch (self->process_index) {
                case 0:
                        sprintf(self->desc->output_buffer, "%08X: ", (uint32_t)self->process_index_item);
                        ush_write_pointer(self, self->desc->output_buffer, self->state);
                        self->process_index = 1;
                        break;
                case 1: {                        
                        self->desc->output_buffer[0] = '\0';
                        for (size_t i = 0; i < 16; i++) {
                                size_t offset = self->process_index_item + i;
                                if (offset < self->process_data_size) {
                                        uint8_t b = self->process_data[offset];
                                        sprintf(&self->desc->output_buffer[strlen(self->desc->output_buffer)], "%02X ", b);
                                } else {
                                        strcat(self->desc->output_buffer, "   ");
                                }
                        }
                        strcat(self->desc->output_buffer, " ");
                        ush_write_pointer(self, self->desc->output_buffer, self->state);
                        self->process_index = 2;
                        break;
                }
                case 2:{
                        self->desc->output_buffer[0] = '\0';
                        for (size_t i = 0; i < 16; i++) {
                                size_t offset = self->process_index_item;
                                if (offset < self->process_data_size) {
                                        uint8_t b = self->process_data[offset];
                                        if (ush_utils_is_printable(b) != false) {
                                                sprintf(&self->desc->output_buffer[strlen(self->desc->output_buffer)], "%c", (char)b);
                                        } else {
                                                strcat(self->desc->output_buffer, ".");
                                        }
                                } else {
                                        strcat(self->desc->output_buffer, " ");
                                }
                                self->process_index_item++;
                        }
                        ush_write_pointer(self, self->desc->output_buffer, self->state);
                        self->process_index = 3;
                        break;
                }
                case 3:
                        if (self->process_index_item >= self->process_data_size) {
                                ush_write_pointer(self, "\r\n", USH_STATE_PROCESS_FINISH);
                                break;
                        }
                        ush_write_pointer(self, "\r\n", self->state);
                        self->process_index = 0;
                        break;
                default:
                        USH_ASSERT(false);
                        break;
                }
                break;
        }

        case USH_STATE_PROCESS_FINISH:
                self->state = USH_STATE_RESET_PROMPT;
                break;

        default:
                processed = false;
                break;
        }

        return processed;
}