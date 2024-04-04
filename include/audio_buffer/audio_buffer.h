#ifndef INCLUDE_AUDIO_BUFFER_AUDIO_BUFFER_H
#define INCLUDE_AUDIO_BUFFER_AUDIO_BUFFER_H
#include <stddef.h>
#include <stdbool.h>
struct rdx_audio_buffer;
typedef struct rdx_audio_buffer rdx_audio_buffer;

typedef enum rdx_audio_buffer_status {
    AB_OK = 0,
    AB_UKNWN_ERR,
    AB_ERR_FILE_ACCESS,
    AB_ERR_UNKNWN_FILE_FMT,
    AB_ERR_NO_MEM,
} rdx_audio_buffer_status;

/// @brief 
/// @param buffer_size_in_bytes 
/// @param read_interleaved whether to read channel sample data interleaved (ch0,ch1,ch2,ch0,ch1,ch2...) or not (ch0,ch0,ch0,ch1,ch1,ch1...)
/// @param alloc 
/// @param dealloc 
/// @return 
rdx_audio_buffer* rdx_create_audio_stream(size_t buffer_size_in_bytes, bool read_interleaved, void* (*alloc)(size_t),void (*dealloc)(void*));
/// @brief 
/// @param strm 
/// @param file 
/// @param out_data 
/// @param out_data_size 
/// @return newly created buffer
rdx_audio_buffer_status rdx_open(rdx_audio_buffer* buf, const char* file, void** out_data, size_t* out_data_size);
/// @brief 
/// @param strm 
/// @return 
rdx_audio_buffer_status rdx_close(rdx_audio_buffer* buf);
/// @brief 
/// @param strm 
/// @param out_data 
/// @param out_data_size 
/// @return 
rdx_audio_buffer_status rdx_fill_next_buffer(rdx_audio_buffer* buf, void** out_data, size_t* out_data_size);
/**
 * Destroys the audio stream and the associated resources
 * @param[in] strm the stream to destroy
 * @pre strm must be a valid audio_stream created with @ref create_audio_stream
*/
void rdx_destroy_audio_stream(rdx_audio_buffer* buf);
#endif