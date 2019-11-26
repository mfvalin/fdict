interface
  function fasthash64(data, l) result(hash) bind(C,name='FastHash64')
    import :: C_PTR, C_INT32_T, C_INT64_T
    type(C_PTR), intent(IN), value :: data
    integer(C_INT32_T), intent(IN), value :: l
    integer(C_INT64_T) :: hash
  end function FastHash64
  function hash_lookup(c, key, keylen, datalen) result(data) bind(C,name='HashLookup')
    import :: C_PTR, C_INT32_T
    type(C_PTR), intent(IN), value :: c, key
    integer(C_INT32_T), intent(IN), value :: keylen
    integer(C_INT32_T), intent(OUT) :: datalen
    type(C_PTR) :: data
  end function hash_lookup
  function hash_insert(c, key, keylen, data, datalen, keycopy) result(status) bind(C,name='HashInsert')
    import :: C_PTR, C_INT32_T
    type(C_PTR), intent(IN), value :: c, key, data
    integer(C_INT32_T), intent(IN), value :: keylen, datalen, keycopy
    integer(C_INT32_T) :: status
  end function hash_insert
  function create_hash_context(n) result(context) bind(C,name='CreateHashContext')
    import :: C_PTR, C_INT32_T
    integer(C_INT32_T), intent(IN), value :: n
    type(C_PTR) :: context
  end function create_hash_context
  subroutine print_hash_context_stats(context, header) bind(C,name='PrintHashContextStats')
    import :: C_PTR, C_CHAR
    type(C_PTR), intent(IN), value :: context
    character(C_CHAR), dimension(*) :: header
  end subroutine print_hash_context_stats
end interface
