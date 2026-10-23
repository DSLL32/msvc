/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   %     Copyright (C) 1994, by WATCOM International Inc.  All rights    %
   %     reserved.  No part of this software may be reproduced or        %
   %     used in any form or by any means - graphic, electronic or       %
   %     mechanical, including photocopying, recording, taping or        %
   %     information storage and retrieval systems - except with the     %
   %     written permission of WATCOM International Inc.                 %
   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

#include <stdlib.h>
#include <wcdefs.h>
#include <wcexcept.h>



//
// Throw an empty_container exception if it is enabled
//

void WCExcept::base_throw_empty_container() const {
#ifdef __SW_XS
    if( (exceptions() & empty_container) != 0 ) {
        failure oops( empty_container );
        throw( oops );
    }
#endif
}



//
// Throw an index_range exception if it is enabled
//

void WCExcept::base_throw_index_range() const {
#ifdef __SW_XS
    if( (exceptions() & index_range) != 0 ) {
        failure oops( index_range );
        throw( oops );
    }
#endif
}



//
// Throw a not_empty exception if it is enabled
//

void WCExcept::base_throw_not_empty() {
#ifdef __SW_XS
    if( (exceptions() & not_empty) != 0 ) {
        failure oops( not_empty );
        // to help prevent more exceptions being thrown
        exception_flags = 0;
        throw( oops );
    }
#endif
}



//
// Throw a not_unique exception if it is enabled
//

void WCExcept::base_throw_not_unique() const {
#ifdef __SW_XS
    if( (exceptions() & not_unique) != 0 ) {
        failure oops( not_unique );
        throw( oops );
    }
#endif
}



//
// Throw an out_of_memory exception if it is enabled
//

void WCExcept::base_throw_out_of_memory() const {
#ifdef __SW_XS
    if( (exceptions() & out_of_memory) != 0 ) {
        failure oops( out_of_memory );
        throw( oops );
    }
#endif
}



//
// Throw a resize_required exception if it is enabled
//

void WCExcept::base_throw_resize_required() const {
#ifdef __SW_XS
    if( (exceptions() & resize_required) != 0 ) {
        failure oops( resize_required );
        throw( oops );
    }
#endif
}



//
// Throw a zero_buckets exception if it is enabled
//

void WCExcept::base_throw_zero_buckets() const {
#ifdef __SW_XS
    if( (exceptions() & zero_buckets) != 0 ) {
        failure oops( zero_buckets );
        throw( oops );
    }
#endif
}



//
// Throw a iter_range exception if it is enabled
//

void WCIterExcept::base_throw_iter_range() const {
#ifdef __SW_XS
    if( (exceptions() & iter_range) != 0 ) {
        failure oops( iter_range );
        throw( oops );
    }
#endif
}



//
// Throw a undef_item exception if it is enabled
//

void WCIterExcept::base_throw_undef_item() const {
#ifdef __SW_XS
    if( (exceptions() & undef_item) != 0 ) {
        failure oops( undef_item );
        throw( oops );
    }
#endif
}



//
// Throw a undef_iter exception if it is enabled
//

void WCIterExcept::base_throw_undef_iter() const {
#ifdef __SW_XS
    if( (exceptions() & undef_iter) != 0 ) {
        failure oops( undef_iter );
        throw( oops );
    }
#endif
}
