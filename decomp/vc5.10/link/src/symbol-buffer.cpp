#include <new.h>
#include "symbol-buffer.h"
#include <recon.h>

ScopedBuffer::~ScopedBuffer()
{
    if (data) operator delete(data);
}
