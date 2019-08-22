program test
  use ISO_C_BINDING
  implicit none
  include 'fast_hash.inc'

  type(C_PTR) :: context
  integer(C_INT32_T) :: status
  integer(C_INT64_T) :: hash
  character(len=32), target :: str
  integer, dimension(128), target :: data
  integer :: iun, nlines
  character(len=128), target :: line

  context = create_hash_context(512000)
  call print_hash_context_stats(context, "Blah Blah Blah"//achar(0))

  iun = 100
  nlines = 0
  open(100,file='test_keys.txt',FORM='FORMATTED')
1 continue
  read(100,'(A128)',end=2,err=2) line
  nlines = nlines+1
  if(nlines > 500000) goto 2
  str = trim(line)//achar(0)
  status = hash_insert(context, C_LOC(str(1:1)), len(trim(line)), C_LOC(data(1)), 512, 1)
  goto 1
2 continue
  print *,'lines read =',nlines

  call print_hash_context_stats(context, "Blah Blah Blah"//achar(0))
  stop

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
