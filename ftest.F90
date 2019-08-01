program test
  use ISO_C_BINDING
  implicit none
  include 'fast_hash.inc'

  type(C_PTR) :: context
  integer(C_INT32_T) :: status
  integer(C_INT64_T) :: hash
  character(len=32), target :: str
  integer, dimension(128), target :: data

  context = create_hash_context(1024)
  str = 'taratata shimboum tsoin tsoin'
  hash = fasthash64(C_LOC(str(1:1)), 32)
  print 100,'hash of '//trim(str)//' =',hash
  status = hash_insert(context, C_LOC(str(1:1)), 32, C_LOC(data(1)), 512, 0)

100 format(A,Z20.16)
  
end program
