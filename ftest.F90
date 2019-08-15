program test
  use ISO_C_BINDING
  implicit none
  include 'fast_hash.inc'

  type(C_PTR) :: context
  integer(C_INT32_T) :: status
  integer(C_INT64_T) :: hash
  character(len=32), target :: str
  integer, dimension(128), target :: data

  context = create_hash_context(1000)
  str = 'taratata shimboum tsoin tsoin'
  hash = fasthash64(C_LOC(str(1:1)), 32)
  print 100,'hash of '//trim(str)//' =',hash
  status = hash_insert(context, C_LOC(str(1:1)), len(trim(str)), C_LOC(data(1)), 512, 1)
  if(status == 0) print 100,'SUCCESS'
  status = hash_insert(context, C_LOC(str(1:1)), len(trim(str)), C_LOC(data(1)), 512, 1)
  if(status /= 0) print 100,'DUPLICATE'
  status = hash_insert(context, C_LOC(str(1:1)), len(trim(str)), C_LOC(data(1)), 512, 1)
  if(status /= 0) print 100,'DUPLICATE'

  call print_hash_context_stats(context, "Blah Blah Blah"//achar(0))

100 format(A,Z20.16)
  
end program
