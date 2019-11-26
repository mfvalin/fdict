interface
!!****f* hash_functions/fasthash64
!! Synopsis
!!   compute a 64 bit hash for a sequence of bytes
!! ARGUMENTS
  function fasthash64(data, l) result(hash) bind(C,name='FastHash64')
    import :: C_PTR, C_INT32_T, C_INT64_T
    type(C_PTR), intent(IN), value :: data            ! pointer to data
    integer(C_INT32_T), intent(IN), value :: l        ! length in bytes of data
    integer(C_INT64_T) :: hash
!!****
  end function FastHash64
!!****f* hash_functions/hash_lookup
!! Synopsis
!!   get data associated with a key from a hash context (obtained from create_hash_context)
!! ARGUMENTS
  function hash_lookup(c, key, keylen, datalen) result(data) bind(C,name='HashLookup')
    import :: C_PTR, C_INT32_T
    type(C_PTR), intent(IN), value :: c              ! pointer to hash context (from create_hash_context)
    type(C_PTR), intent(IN), value :: key            ! byte sequence used as a key
    integer(C_INT32_T), intent(IN), value :: keylen  ! length of key (bytes)
    integer(C_INT32_T), intent(OUT) :: datalen       ! length of data (-1 if error) (bytes)
    type(C_PTR) :: data                              ! pointer to data (C_NULL_PTR if key not found or error)
!!****
  end function hash_lookup
!!****f* hash_functions/hash_insert
!! Synopsis
!!   insert key and data in a hash context (obtained from create_hash_context)
!! ARGUMENTS
  function hash_insert(c, key, keylen, data, datalen, keycopy) result(status) bind(C,name='HashInsert')
    import :: C_PTR, C_INT32_T
    type(C_PTR), intent(IN), value :: c              ! pointer to hash context (from create_hash_context)
    type(C_PTR), intent(IN), value :: key            ! byte sequence used as a key
    type(C_PTR), intent(IN), value :: data           ! pointer to data associated with key
    integer(C_INT32_T), intent(IN), value :: keylen  ! length of key (bytes)
    integer(C_INT32_T), intent(IN), value :: datalen ! length of data (bytes)
    integer(C_INT32_T), intent(IN), value :: keycopy ! if nonzero, save a copy of the key
    integer(C_INT32_T) :: status
!!****
  end function hash_insert
!!****f* hash_functions/create_hash_context
!! Synopsis
!!   create a hash context (used with hash_insert and hash_lookup)
!! ARGUMENTS
  function create_hash_context(n) result(context) bind(C,name='CreateHashContext')
    import :: C_PTR, C_INT32_T
    integer(C_INT32_T), intent(IN), value :: n       ! number of keys expected
    type(C_PTR) :: context                           ! context (used by hash_insert and hash_lookup)
!!****
  end function create_hash_context
!!****f* hash_functions/print_hash_context_stats
!! Synopsis
!!   print statistics associated with a hash context 
!! ARGUMENTS
  subroutine print_hash_context_stats(context, header) bind(C,name='PrintHashContextStats')
    import :: C_PTR, C_CHAR
    type(C_PTR), intent(IN), value :: context        ! context (used by hash_insert and hash_lookup)
    character(C_CHAR), dimension(*) :: header        ! header string for listing
!!****
  end subroutine print_hash_context_stats
end interface
