struct AliasBlock {
    int first, second, third;
    unsigned char bytes[12];
};

int pointer_aliasing(struct AliasBlock *destination, struct AliasBlock *source,
    int *left, int *right, int count)
{
    struct AliasBlock saved = *source;
    int before = *left;
    int index;
    destination->first = before + source->second;
    *right = source->third + before;
    destination->second = *left + *right;
    *destination = saved;
    destination->bytes[1] = source->bytes[0];
    for (index = 0; index < count; ++index) {
        int value = left[index];
        right[index] = value + destination->second;
        destination->bytes[index & 7] ^= (unsigned char)*left;
    }
    return before + *left + *right + destination->third;
}
