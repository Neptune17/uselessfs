# uselessfs

to build:

make

to run:

mkdir example
./uselessfs -s -o default_permissions -o auto_unmount example/

to exit:

fusermount -u example
rm -r example