#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <sys/types.h>

/* memory allocation routines */

/** allocate buffer of specified size */
void* get_buffer(const size_t size);

/** free buffer */
void free_buffer(void *buffer);

/** append (copy) data to buffer */
unsigned pack(const void *data, const size_t size, void *buffer, const off_t offset);

/** append (copy) uint16_t to buffer 
@return offset after appended data
*/
unsigned pack_16(const uint16_t *data, void *buffer, const off_t offset);

/** append (copy) uint32_t to buffer 
@return offset after appended data
*/
unsigned pack_32(const uint32_t *data, void *buffer, const off_t offset);

/** append (copy) uint64_t to buffer 
@return offset after appended data
*/
unsigned pack_64(const uint64_t *data, void *buffer, const off_t offset);

/** get (copy) data from buffer 
@return offset after copied data
*/
unsigned unpack(void *data, const size_t size, const void *buffer, const off_t offset);

/** get (copy) uint16_t from buffer 
@return offset after copied data
*/
unsigned unpack_16(uint16_t *data, const void *buffer, const off_t offset);

/** get (copy) uint32_t from buffer 
@return offset after copied data
*/
unsigned unpack_32(uint32_t *data, const void *buffer, const off_t offset);

/** get (copy) uint64_t from buffer 
@return offset after copied data
*/
unsigned unpack_64(uint64_t *data, const void *buffer, const off_t offset);

/** print buffer to output. debug only */
void dump(const void *data, const size_t data_len);

#endif // BUFFER_H
