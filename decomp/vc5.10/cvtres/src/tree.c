#include "cvtres.h"
#include <stdlib.h>
#include <string.h>
#include <recon.h>

ResourceEntry *__stdcall insert_resource(ResourceIdentifier *type,
    ResourceIdentifier *name, ResourceHeader *header)
{
    ResourceType **type_link, *group;
    ResourceEntry **link = NULL, *entry, *first = NULL;
    int ordinal_type = type->kind == 2;
    int ordinal_name = name->kind == 2;

    type_link = ordinal_type ? &resource_id_types : &resource_named_types;
    for (group = *type_link; group; group = *type_link) {
        /* The original compares this union WORD before its named-type path. */
        if (group->identifier->value.ordinal == type->value.ordinal) {
            link = ordinal_name ? &group->ids : &group->names;
            break;
        }
        if (ordinal_type) {
            if (type->value.ordinal < group->identifier->value.ordinal) break;
        } else if (wcscmp(type->text, group->identifier->text) < 0) break;
        type_link = &group->next;
    }
    if (!link) {
        ++type_count;
        if (ordinal_type) ++id_type_count;
        else ++named_type_count;
        group = allocate_block(24);
        group->next = *type_link;
        *type_link = group;
        group->identifier = type;
        link = ordinal_name ? &group->ids : &group->names;
    }
    for (entry = *link; entry; entry = *link) {
        if (ordinal_name) {
            if (name->value.ordinal == entry->name->value.ordinal) {
                first = entry;
                break;
            }
            if (name->value.ordinal < entry->name->value.ordinal) break;
        } else {
            if (!wcscmp(name->text, entry->name->text)) {
                first = entry;
                break;
            }
            if (wcscmp(name->text, entry->name->text) < 0) break;
        }
        link = &entry->next;
    }
    if (first) {
        for (;;) {
            if (header->language == entry->header->language) {
                printf(load_message(8001), 1100, 32);
                if (type->kind == 1) printf(load_message(2000), type->text, 32);
                else if (type->value.ordinal <= 20 && resource_type_names[type->value.ordinal])
                    printf(load_message(2001), resource_type_names[type->value.ordinal], 32);
                else printf(load_message(2002), type->value.ordinal, 32);
                if (name->kind == 1) printf(load_message(2003), name->text, header->language);
                else printf(load_message(2004), name->value.ordinal, header->language);
                exit(1);
            }
            if (header->language < entry->header->language) break;
            entry = entry->next;
            if (!entry || entry->language_count) break;
        }
    }
    entry = allocate_block(32);
    entry->next = *link;
    *link = entry;
    entry->name = name;
    entry->type = type;
    entry->header = header;
    if (first) {
        ++language_entry_count;
        if (first == entry->next) {
            entry->language_count = first->language_count + 1;
            first->language_count = 0;
        } else {
            ++first->language_count;
            entry->language_count = 0;
        }
    } else {
        ++name_entry_count;
        ++language_entry_count;
        ++name_directory_count;
        entry->language_count = 1;
        if (ordinal_name) ++group->id_count;
        else ++group->name_count;
    }
    return entry;
}
