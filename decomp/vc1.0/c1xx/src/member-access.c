/* Class qualification, protection levels and friend access. */
#include "member-access.h"
#include "constructor-initializers.h"
#include <recon.h>

int base_initialization_access; /* original 0x97bc8 */

CppSymbol *access_failure_member; /* original 0x97bd4 */
unsigned char access_failure_protection; /* original 0x97be0 */
CppSymbol *access_failure_owner; /* original 0x97bcc */
int access_through_virtual_base; /* original 0x97bd0 */

/* Original 0x31a50..0x31e7e. Protection levels are private=1, protected=2,
   public=3. A qualification's enclosing paths may lower the effective level;
   friends and access declarations can restore access along those paths. */
int __stdcall qualified_member_is_accessible(void *reference_pointer)
{
    CppSymbol *reference = reference_pointer;
    CppSymbol *scope_class, *associated_class, *target, *node, *owner, *access_owner;
    CppSymbol *first_owner = 0, *previous_base = 0;
    /* Native 0x31c05 loads an otherwise unwritten stack slot before the
       kind-4/kind-15 cases replace it. Preserve its unspecified fallback
       for other path kinds rather than inventing a class identity. */
    CppSymbol *unspecified_owner;
    QualifierBinding *binding;
    unsigned char access, required, base_access, old_access;
    int implicit, static_target = 0, first_owner_is_friend = 0;
    int associated_base = 0, depth = 1, restricted_depth = 0;
    int friend_access = 0, restricted_friend_access = 0;
    unsigned kind;

    if (reference->flags & 0x80) return 1;
    scope_class = (unsigned char)current_scope->flags < 0xfa ? 0 : *current_scope->buckets;
    associated_class = current_class_context < 0 ? declaring_class_for_symbol(function_symbol) :
        get_current_class_symbol();
    binding = ((QualifiedDeclarator *)reference)->resolution->binding;
    reset_member_access_failure();
    target = (CppSymbol *)binding->target;
    if (scope_class && deferred_declarations[declaration_depth] == reference && scope_class == target)
        return 1; /* The native code repeats this identical test. */
    implicit = (reference->declaration_flags & 0x8000) != 0;
    if (implicit && ((target->storage[0] & 7) == 3 ||
        (target->kind & 0x1f) == 5 || (target->flags & 0x10))) static_target = 1;
    if (static_target && (target->kind & 0x1f) == 4 && scope_class == target) return 1;
    required = implicit ? 1 : (get_base_initialization_access() ? 2 : 3);
    access = target->flags & 3;
    binding = binding->next;
    while (binding) {
        node = (CppSymbol *)binding->target;
        kind = node->kind & 0x1f;
        owner = unspecified_owner;
        if (kind == 4) owner = node;
        else if (kind == 15)
            owner = cpp_context_stack->items[((ClassReference *)node)->context_index];
        if (!first_owner) {
            set_initial_member_access_failure(access, target, owner);
            first_owner = owner;
        }
        if (static_target && binding->following == binding)
            return first_owner == owner ? 1 : access > 1;
        if (scope_class == owner) required = 1;
        if (owner->detail.class_info->flags & 0x40) {
            friend_access = 0;
            if (scope_class) friend_access = find_friend_access(scope_class, owner);
            if (!friend_access && owner) friend_access = find_friend_access(associated_class, owner);
            if (friend_access) {
                if (first_owner == owner) first_owner_is_friend = 1;
                required = 1;
            }
        }
        if (node == associated_class) required = 1;
        if (kind != 15) {
            if (first_owner_is_friend && (!restricted_depth ||
                (depth - 1 <= restricted_depth && restricted_friend_access == 1))) return 1;
            if (associated_base) return old_access >= 2;
            return required <= access;
        }
        associated_base = owner == associated_class;
        if (((BaseClassLink *)node)->state & 1) {
            binding = skip_base_qualifier_path(binding);
            base_access = virtual_base_access_level(
                cpp_context_stack->items[((ClassReference *)node)->context_index],
                (CppSymbol *)binding->next->target);
            access_through_virtual_base = 1;
        } else base_access = node->flags & 3;
        if (base_access != 3) {
            access_owner = (CppSymbol *)binding->next->target;
            if ((access_owner->kind & 0x1f) != 4)
                access_owner = cpp_context_stack->items[((ClassReference *)access_owner)->context_index];
            if (has_member_access_declaration(access_owner, (BaseClassLink *)node, target)) base_access = 3;
        }
        if ((base_access == 1 || base_access == 2) && !restricted_depth) {
            restricted_depth = depth;
            restricted_friend_access = friend_access;
        }
        old_access = access;
        access = combine_member_access(access, base_access);
        if (access < old_access && access < required)
            set_inherited_member_access_failure(target, node,
                previous_base ? previous_base->flags & 3 : 0, old_access, previous_base);
        previous_base = node;
        binding = binding->next;
        ++depth;
    }
    return 1;
}

/* Original 0x317d0..0x31800. A zero declaring-class index is the absent
   sentinel; other signed indices use the shared class-context vector. */
CppSymbol *__stdcall declaring_class_for_symbol(CppSymbol *symbol)
{
    short index;
    if (!symbol) return 0;
    index = symbol->auxiliary.declaring_class;
    if (!index) return 0;
    return cpp_context_stack->items[index];
}

/* Original 0x31820..0x31835. Other diagnostic fields are deliberately
   retained until the first owner records the new access failure. */
void reset_member_access_failure(void)
{
    access_failure_owner = 0;
    access_through_virtual_base = 0;
}

/* Original 0x31870..0x3188e. Keep the member and its first enclosing
   class separate from later base-path failure details. */
void __stdcall set_initial_member_access_failure(unsigned char protection,
    CppSymbol *member, CppSymbol *owner)
{
    access_failure_owner = owner;
    access_failure_member = member;
    access_failure_protection = protection;
}

/* Original 0x32060..0x32065. Base initialization permits protected access. */
int get_base_initialization_access(void)
{
    return base_initialization_access;
}

/* Original 0x9ec80. Rows and columns are unavailable/private/protected/
   public. An inherited private member is inaccessible; otherwise the path
   can lower its protection to private or protected. */
const unsigned char inherited_access_levels[4][4] = {
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 1, 2, 2 },
    { 0, 1, 2, 3 }
};

/* Original 0x31800..0x31815. Both access arguments are byte-valued
   stack slots; the original caller supplies the two-bit domain. */
unsigned char __stdcall combine_member_access(unsigned char member_access,
    unsigned char base_access)
{
    return inherited_access_levels[member_access][base_access];
}

/* Original 0x32070..0x3207b. Share the existing access-context word
   read by get_base_initialization_access; it is not a Boolean-normalized flag. */
void __stdcall set_base_initialization_access(int enabled)
{
    base_initialization_access = enabled;
}
