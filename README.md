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
You can now run the script by typing `sd` without needing to specify the full path to the file. But this change is only temporary and valid only in the current shell session.

To make the change permanent, you need to define the $PATH variable in the shell configuration files. This achived by setting the variable in the `~/.bashrc` file. 

4. Open the file with your text editor, here I am using vim  and add the following line at the end of it:
```
vim ~/.bashrc
```
```
export PATH="$HOME/doodad:$PATH"
```
5. Save the file and load the new $PATH into the current shell session using the source command:
```
source ~/.bashrc
```
6. To confirm that the directory was successfully added, print the value of your $PATH by typing:
```
echo $PATH
```
7. Now you can easly run the command and choose you desired programming mode 
```
sd -ph
```


All rights reserved to Danny Chouinard
 [ Danny Chouinard's doodads](https://sites.google.com/site/dannychouinard/Home/atmel-avr-stuff/sd)
