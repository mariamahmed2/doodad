# Serial Doodad
##  Serial Doodad: Simplest way to program a micro-controller

This is an approach to program a micro-controller without a programmer.

1. Download the attached `sd.c` file
2. Compile it using 
```
gcc sd.c -o sd
```
An excutable file named `sd` will appeare, this file is used by the shell to run any `sd` commands, the shell searches through all directories specified in the user `$PATH` variable for an executable file of `sd` name.

To do this, make a directory, copy the excutable file `sd` into it, theh make that dir pointed by your `PATH`.

3. Let’s say you make a directory called `doodad` located in your Home directory
```
export PATH="$HOME/doodad:$PATH"
```
You can now run the script by typing `sd` without needing to specify the full path to the file.
