Some tips on how to run this.

#Untar the reference output
mkdir refout
(cd refout; tar -tzvf ../refout.tar.Z )

#Optional Untar the verbose reference output
mkdir refout_v
( cd refout_v ; tar -tzvf ../refout_verbose.tar.Z )

#write your program .. standard way of writing a program
mkdir src
vi sched.c
make 

# run all examples ... assuming your executable is in ./src/sched
# make a outputdir
mkdir outputdir
./runit.sh outputdir ./src/sched

# compare outputs with reference output

./diffit.sh refout outputdir

You might get something like this.


frankeh@NYU2:~/lab2/lab_assign$ ./diffit.sh ../refout ../myout/
************  outputs1_F  differ
7c7
< SUM: 8237 12.14 6.53 307.60 0.00 0.061
---
> SUM: 8237 0.14 6.53 307.60 0.00 0.061
************  outputs4_P2  differ
6c6
< SUM: 1109 46.89 85.57 570.50 36.00 0.361
---
> SUM: 1109 46.89 85.67 570.50 36.00 0.361


In that case you need to inspect the outputs and determine why you get
different results.
You might want to run with -v option and compare in detail one particular
output and go from there.


Finally: 

outformat.c  provides the code and verifcation for the output.
You should use those printf formats or convert to C++ (note C++ accepts printf !!! ).

