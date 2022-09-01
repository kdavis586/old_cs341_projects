# Welcome to Homework 0!

For these questions you'll need the mini course and  "Linux-In-TheBrowser" virtual machine (yes it really does run in a web page using javascript) at -

http://cs-education.github.io/sys/

Let's take a look at some C code (with apologies to a well known song)-
```C
// An array to hold the following bytes. "q" will hold the address of where those bytes are.
// The [] mean set aside some space and copy these bytes into teh array array
char q[] = "Do you wanna build a C99 program?";

// This will be fun if our code has the word 'or' in later...
#define or "go debugging with gdb?"

// sizeof is not the same as strlen. You need to know how to use these correctly, including why you probably want strlen+1

static unsigned int i = sizeof(or) != strlen(or);

// Reading backwards, ptr is a pointer to a character. (It holds the address of the first byte of that string constant)
char* ptr = "lathe"; 

// Print something out
size_t come = fprintf(stdout,"%s door", ptr+2);

// Challenge: Why is the value of away equal to 1?
int away = ! (int) * "";


// Some system programming - ask for some virtual memory

int* shared = mmap(NULL, sizeof(int*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
munmap(shared,sizeof(int*));

// Now clone our process and run other programs
if(!fork()) { execlp("man","man","-3","ftell", (char*)0); perror("failed"); }
if(!fork()) { execlp("make","make", "snowman", (char*)0); execlp("make","make", (char*)0)); }

// Let's get out of it?
exit(0);
```

## So you want to master System Programming? And get a better grade than B?
```C
int main(int argc, char** argv) {
	puts("Great! We have plenty of useful resources for you, but it's up to you to");
	puts(" be an active learner and learn how to solve problems and debug code.");
	puts("Bring your near-completed answers the problems below");
	puts(" to the first lab to show that you've been working on this.");
	printf("A few \"don't knows\" or \"unsure\" is fine for lab 1.\n"); 
	puts("Warning: you and your peers will work hard in this class.");
	puts("This is not CS225; you will be pushed much harder to");
	puts(" work things out on your own.");
	fprintf(stdout,"This homework is a stepping stone to all future assignments.\n");
	char p[] = "So, you will want to clear up any confusions or misconceptions.\n";
	write(1, p, strlen(p) );
	char buffer[1024];
	sprintf(buffer,"For grading purposes, this homework 0 will be graded as part of your lab %d work.\n", 1);
	write(1, buffer, strlen(buffer));
	printf("Press Return to continue\n");
	read(0, buffer, sizeof(buffer));
	return 0;
}
```
## Watch the videos and write up your answers to the following questions

**Important!**

The virtual machine-in-your-browser and the videos you need for HW0 are here:

http://cs-education.github.io/sys/

The coursebook:

http://cs341.cs.illinois.edu/coursebook/index.html

Questions? Comments? Use Ed: (you'll need to accept the sign up link I sent you)
https://edstem.org/

The in-browser virtual machine runs entirely in Javascript and is fastest in Chrome. Note the VM and any code you write is reset when you reload the page, *so copy your code to a separate document.* The post-video challenges (e.g. Haiku poem) are not part of homework 0 but you learn the most by doing (rather than just passively watching) - so we suggest you have some fun with each end-of-video challenge.

HW0 questions are below. Copy your answers into a text document (which the course staff will grade later) because you'll need to submit them later in the course. More information will be in the first lab.

## Chapter 1

In which our intrepid hero battles standard out, standard error, file descriptors and writing to files.

### Hello, World! (system call style)
1. Write a program that uses `write()` to print out "Hi! My name is `<Your Name>`".
```C
// ANSWER
#include <unistd.h>

int main() {
	write(1, "Hi! My name is Kaelan\n", 22);
	return 0;
}
```
### Hello, Standard Error Stream!
2. Write a function to print out a triangle of height `n` to standard error.
   - Your function should have the signature `void write_triangle(int n)` and should use `write()`.
   - The triangle should look like this, for n = 3:
   ```C
   *
   **
   ***
   ```

   ```C
   // ANSWER
   #include <unistd.h>

	void write_triangle(int n) {
		char arr[n];
		int i;
		for (i = 0; i < n; i++) {
			arr[i] = '*';
			write(STDERR_FILENO, arr, i+1);
			write(STDERR_FILENO, "\n", 1);
		}
	}

	int main() {
		write_triangle(3);
		return 0;
	}
   ```
### Writing to files
3. Take your program from "Hello, World!" modify it write to a file called `hello_world.txt`.
   - Make sure to to use correct flags and a correct mode for `open()` (`man 2 open` is your friend).
   
	```C
	// ANSWER
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>

	int main() {
		mode_t mode = S_IRUSR | S_IWUSR;
		int filedesc = open("hello_world.txt", O_CREAT | O_TRUNC | O_RDWR, mode);
		write(filedesc, "Hi! My name is Kaelan\n", 22);
		return 0;
	}
	```
### Not everything is a system call
4. Take your program from "Writing to files" and replace `write()` with `printf()`.
   - Make sure to print to the file instead of standard out!
	```C
	// ANSWER
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <stdio.h>

	int main() {
		mode_t mode = S_IRUSR | S_IWUSR;
		close(STDOUT_FILENO);
		open("hello_world.txt", O_CREAT | O_TRUNC | O_RDWR, mode);
		printf("Hi! My name is Kaelan\n");
		return 0;
	}
	```
5. What are some differences between `write()` and `printf()`?
   <br>(ANSWER)<br>
   1. printf is buffered, not immediately showing the output, write is not.
   2. printf will write to whatever stream is assigned to the integer value 1.
   3. printf can format different primitives into the output.

## Chapter 2

Sizing up C types and their limits, `int` and `char` arrays, and incrementing pointers

### Not all bytes are 8 bits?
1. How many bits are there in a byte?
   1. (ANSWER) There is a minimum of 8 bits in a byte (but not all bytes are 8 bits).
2. How many bytes are there in a `char`?
   1. (ANSWER) A char is always one byte.
3. How many bytes the following are on your machine?
   - `int`, `double`, `float`, `long`, and `long long`
   <br>(ANSWER)<br>
      - (On my machine) int -> 4 bytes
      - (On my machine) double -> 8 bytes
      - (On my machine) float -> 4 bytes
      - (On my machine) long -> 4 bytes
      - (On my machine) long long -> 8 bytes
### Follow the int pointer
4. On a machine with 8 byte integers:
```C
int main(){
    int data[8];
} 
```
If the address of data is `0x7fbd9d40`, then what is the address of `data+2`?
<br>&nbsp;&nbsp;&nbsp;&nbsp;(ANSWER) `0x7fbd9d50`<br>
5. What is `data[3]` equivalent to in C?
   - Hint: what does C convert `data[3]` to before dereferencing the address?
<br>(ANSWER) `*(data + 3)`<br>
### `sizeof` character arrays, incrementing pointers
  
Remember, the type of a string constant `"abc"` is an array.

6. Why does this segfault?
```C
char *ptr = "hello";
*ptr = 'J';
```
(ANSWER) The string "hello" is a constant and therefore is read-only memory, when we attempt to change this read only memory, since that is not allowed, we segfault.<br><br>

7. What does `sizeof("Hello\0World")` return?
   1. (ANSWER) 12
8. What does `strlen("Hello\0World")` return?
   1. (ANSWER) 5
9.  Give an example of X such that `sizeof(X)` is 3.
    1.  (ANSWER) `X = "hi";`
10. Give an example of Y such that `sizeof(Y)` might be 4 or 8 depending on the machine.
    1.  (ANSWER) `Y = 1;`

## Chapter 3

Program arguments, environment variables, and working with character arrays (strings)

### Program arguments, `argc`, `argv`
1. What are two ways to find the length of `argv`?
   1. (ANSWER) The length of `argv` is `argc`. It can also be found by using the following code:<br>
   ```C
	int counter = 0;
	while (argv[counter]) {
		counter++;
	}
	// argv[argc] is the null pointer, so this will always terminate with the correct count of args
   ```
2. What does `argv[0]` represent?
   1. (ANSWER) The name of the program that is going to be run.
### Environment Variables
3. Where are the pointers to environment variables stored (on the stack, the heap, somewhere else)?
   1. (ANSWER) Environment Variables are stored on the system separate from the program running and can be used by any other programs on the system as well.
### String searching (strings are just char arrays)
4. On a machine where pointers are 8 bytes, and with the following code:
```C
char *ptr = "Hello";
char array[] = "Hello";
```
What are the values of `sizeof(ptr)` and `sizeof(array)`? Why?
<br>
(ANSWER) `sizeof(ptr) = 4` because this statement is asking for the size of a pointer type, rather than the size of all the characters in "Hello". `sizeof(array) = 6` because we get the amount of bytes allocated for all the characters in this array, including the terminal character `\0`.
<br>

### Lifetime of automatic variables

5. What data structure manages the lifetime of automatic variables?
   1. (ANSWER) Stack.

## Chapter 4

Heap and stack memory, and working with structs

### Memory allocation using `malloc`, the heap, and time
1. If I want to use data after the lifetime of the function it was created in ends, where should I put it? How do I put it there?
   1. (ANSWER) You can store the desired variable onto heap memory. This can be done by using the `malloc` function to get a pointer to allocated heap memory, then copying the desired value into that memorym, then returning the pointer to the heap memory outside of the function.
2. What are the differences between heap and stack memory?
   1. (ANSWER) Stack memory is used for local variables such as those defined inside of functions. These allocations are freed once the function is completed and the variables go out of scope. Heap memory allows for longer term storage of variables that can be accessed and used until it is explicitly freed.
3. Are there other kinds of memory in a process?
   1. (ANSWER) Yes, one example is the call stack that keeps track of all the functions currently running.
4. Fill in the blank: "In a good C program, for every malloc, there is a `free`".
### Heap allocation gotchas
5. What is one reason `malloc` can fail?
   1. (ANSWER) `malloc` may try to allocate more memory than is available.
6. What are some differences between `time()` and `ctime()`?
   1. (ANSWER) `time()` gets a `time_t` value that represents the time since Jan 1, 1970 while `ctime()` takes this value and formats it into a string.
7. What is wrong with this code snippet?
```C
free(ptr);
free(ptr);
```
(ANSWER) Double free of the same pointer. The second free is now trying to free memory that is unknown.
<br>

8. What is wrong with this code snippet?
```C
free(ptr);
printf("%s\n", ptr);
```
(ANSWER) `printf` is trying to access the value of a pointer that has been freed and now has a non-deterministic value.
<br>

9. How can one avoid the previous two mistakes? 
   1.  (ANSWER) These mistakes can be avoided by immediately setting the `ptr` variable to `NULL` to avoid having a dangling pointer.
### `struct`, `typedef`s, and a linked list
10. Create a `struct` that represents a `Person`. Then make a `typedef`, so that `struct Person` can be replaced with a single word. A person should contain the following information: their name (a string), their age (an integer), and a list of their friends (stored as a pointer to an array of pointers to `Person`s).
11. Now, make two persons on the heap, "Agent Smith" and "Sonny Moore", who are 128 and 256 years old respectively and are friends with each other.
    ```C
	// ANSWER
	// This code does not do memory freeing because of the questions below covering that.
	typedef struct Person person;

	struct Person {
		char * name;
		int age;
		person** friends;
	};


	int main() {
		person * smith = (person *) malloc(sizeof(person));
		person * moore = (person *) malloc(sizeof(person));
		
		smith->name = "Agent Smith";
		smith->age = 128;
		smith->friends = (person **) malloc(sizeof(person));
		smith->friends[0] = moore;
		
		moore->name = "Sonny Moore";
		moore->age = 256;
		moore->friends = (person **) malloc(sizeof(person));
		moore->friends[0] = smith;
		
		return 0;
	}
	```
### Duplicating strings, memory allocation and deallocation of structures
Create functions to create and destroy a Person (Person's and their names should live on the heap).

12. `create()` should take a name and age. The name should be copied onto the heap. Use malloc to reserve sufficient memory for everyone having up to ten friends. Be sure initialize all fields (why?).
13. `destroy()` should free up not only the memory of the person struct, but also free all of its attributes that are stored on the heap. Destroying one person should not destroy any others.

```C
// ANSWER
person * create(char * name, int age) {
	person * new_person = (person *) malloc(sizeof(person));
	
	new_person->name = malloc(strlen(name) + 1);
	strcpy(new_person->name, name);
	
	new_person->age = age;
	
	new_person->friends = (person **) malloc(sizeof(person) * 10);
	
	return new_person;
}

void destroy(person * to_destroy) {
	free(to_destroy->name);
	free(to_destroy->friends);
	free(to_destroy);
	to_destroy = NULL;
}
```
## Chapter 5 

Text input and output and parsing using `getchar`, `gets`, and `getline`.

### Reading characters, trouble with gets
1. What functions can be used for getting characters from `stdin` and writing them to `stdout`?
   1. (ANSWER) `getchar` and `putchar` respectively.
2. Name one issue with `gets()`.
   1. (ANSWER) It allows for the possibility of buffer overflow if the input to `gets` is too long to the preallocated buffer.
### Introducing `sscanf` and friends
3. Write code that parses the string "Hello 5 World" and initializes 3 variables to "Hello", 5, and "World".
	```C
	// ANSWER
	int main() {
		char word1[6];
		char word2[6];
		int val = -1;
		
		sscanf("Hello 5 World", "%s %d %s", word1, &val, word2);
		
		printf("%s %d %s\n", word1, val, word2);
		
		return 0;
	}
	```
### `getline` is useful
4. What does one need to define before including `getline()`?
   1. (ANSWER) `_GNU_SOURCE`
5. Write a C program to print out the content of a file line-by-line using `getline()`.
   ```C
   // Answer
   #define _GNU_SOURCE

	#include <stdio.h>
	#include <stdlib.h>

	int main() {
		char * buffer = NULL;
		size_t capacity = 0;
		ssize_t result = 0;
		
		FILE * fptr = fopen("test.txt", "r");
		
		while ((result=getline(&buffer, &capacity, fptr)) != -1) {
			printf("%d : %s", result, buffer);
		}
		
		fclose(fptr);
		return 0;
	}
   ```

## C Development

These are general tips for compiling and developing using a compiler and git. Some web searches will be useful here

1. What compiler flag is used to generate a debug build?
   1. (ANSWER) for gcc, the `-g` flag.
2. You modify the Makefile to generate debug builds and type `make` again. Explain why this is insufficient to generate a new build.
   1. (ANSWER) `make` by itself will still create a build in the same fashion without the extra flags. You must use `make debug` to build in debug mode.
3. Are tabs or spaces used to indent the commands after the rule in a Makefile?
   1. (ANSWER) tabs
4. What does `git commit` do? What's a `sha` in the context of git?;
   1. (ANSWER) It captures a snapshot of the project's currently staged changes. `sha` relates to the unique hash that is generated for each commit. It can be used incase a revert is needed and for identifying different commits in general.
5. What does `git log` show you?
   1. (ANSWER) `git log` shows all the commits in a respository's history.
6. What does `git status` tell you and how would the contents of `.gitignore` change its output?
   1. (ANSWER) `git status` displays the state of the working directory and the staging area i.e. all of the modified files and the files ready for a push. `.gitignore` can change the output of `git status` becuase it allows for the ignoring of any changes to files that are specified in the `.gitignore`.
7. What does `git push` do? Why is it not just sufficient to commit with `git commit -m 'fixed all bugs' `?
   1. (ANSWER) `git push` Pushes all of the committed changes from the local repository to the remote repository. `fixed all bugs` is not a good commit message because it does not explain anything about what specifically was fixed. It also may potentially be a commit with too many changes at once, making it hard to fix if there are issues with it down the line.
8. What does a non-fast-forward error `git push` reject mean? What is the most common way of dealing with this?
   1. (ANSWER) This means that git cannot commit the changes to the remote repository. This can most commonly be fixed by fetching and merging any new changes from the remote repository to the local once, then trying again.

## Optional (Just for fun)
- Convert your a song lyrics into System Programming and C code and share on Ed.
- Find, in your opinion, the best and worst C code on the web and post the link to Ed.
- Write a short C program with a deliberate subtle C bug and post it on Ed to see if others can spot your bug.
- Do you have any cool/disastrous system programming bugs you've heard about? Feel free to share with your peers and the course staff on Ed.
