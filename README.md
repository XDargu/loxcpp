# LoxCpp

This is a simple C++ implementation of the Lox programming language from [Crafting Interpreters](https://craftinginterpreters.com/) with a few extra features.

LoxCpp is a dynamically typed, garbage collected language. It supports both functional patterns and the basics of object orientation.
LoxCpp differs from Lox in some areas and have several extra features, since I wanted to take the language in a different direction. The focus is still on a simple syntax, easy to learn and understand at a glance.

Let's start with the basics. This is how you do a simple Hello World in LoxCpp:

```
// Hello World in LoxCpp
print "Hello, world!";
```

Moving on to something a bit more complex, this is a simple implementation of the FizzBuzz program in LoxCpp:

```
// FizzBuzz in LoxCpp
for i in 1..20
    match i {
        n if n % 15 == 0: print "FizzBuzz";
        n if n % 5 == 0: print "Buzz";
        n if n % 3 == 0: print "Fizz";
        n: print n;
    }
```

Let's look at a classic recursion example, calculating the Nth fibonacci number:

```
fun fib(n)
{
    if (n < 2) return n;
    else return fib(n-1) + fib(n-2);
}
```

Let's have a look at some of the main differences between Lox and LoxCpp!

# Const
LoxCpp has two different ways of declaring variables: **var** and **const**. Const, as you have probably guessed, doesn't allow changes to the value after its declaration.

```
// Compiles correctly
var a = 10;
a = 2;

// Compilation error: "Can't reassign a const variable"
const b = 10;
b = 2;
```

## Lists
Lox Cpp supports basic lists. A list is simply an ordered collection of elements. Lists in LoxCpp are not strongly typed, which means you can add pretty much anything you want to a list:

```
const list = [1, 50, "Hello World", nil, 256];
```

You can access list elements using square brackets, as it's common in many other languages:

```
var list = [1, 50, "Hello World", nil, 256];

// Prints 1
print list[0]

// Prints "Hello World"
print list[2]

list[2] = 20;
// Prints 20
print list[2];
```

You can add or remove values with the **push** and **pop** operations:

```
var list = [];

push(list, 5);
push(list, 10);
// Prints [5, 10]
print list;

const value = pop(list);
// Prints 10
print value;
// Prints [5]
print list;
```

## For-In

On top of the basic loops, LoxCpp adds the for-in loop, which allows iterating over an iterable type, and performing operations on each value. This works for lists, ranges or strings.

```
const name = "Daniel";

// Prints "D" "a" "n" "i" "e" "l" on different lines
for c in name
  print c;
```

```
// Prints 5 6 7 8 on different lines
for c in [5, 6, 7, 8]
  print c;
```

## Ranges
Ranges are a neat type that allows representation of a range of values without actually having to keep them in memory, like in the case of lists.

For example, if you want to make a loop that interates on all numbers between 1 and 100, you could simply do it by using a range. Unlike a list, the range will not allocate memory for each value, it will simply use an iterator to visit all values, one at a time.

```
for i in 1..100
  print i;
```

Ranges can go in both directions: increasing or decreasing values:

```
for i in 100..1
  print i;
```

## Anonymous functions
Anonymous functions or "lambda functions" allow the creation of functions without giving them a name or assigning them to a variable.

They are commonly used when calling higher-order functions.

Lox Cpp supports anonymous functions with the same syntax as normal function declaration, using the keyworkd **fun**. One of the key ideas of LoxCpp is ensure there are not several ways of doing essentially the same thing (ahem C++...).

This is a simple example of getting the even numbers from a list with both a named and an annonymous function:

```
const values = [1, 50, 77, 256];

// Using an annonymous function
const evenNumbers = filter(values, fun(num){ return num % 2 == 0; });

// Using a named function
fun isEven(num) { return num % 2 == 0; };
const evenNumbers2 = filter(values, isEven);

// Both print [50, 256]
print evenNumbers;
print evenNumbers2;
```

## Pattern matching

LoxCpp has basic pattern matching by using the **match** keyword. You can match a value to any value. If the values are equal, the code on the right of the pattern will be executed:

```
const value = 5;
match value {
        5: print "This is the number 5";
        10: print "This is the number 10";
        "Hello World": print "This is the string Hello World";
    }
```

There are a few neat things you can do with pattern matching.

You can match ranges:

```
match(value)
{
    1..10: print "Value from 1 to 10";
    20: print "That's a 20";
}
```

You can capture the value you are matching, to then use it later, by declaring a named variable as the pattern:

```
const value = 5;
match value {
        5: print "This is the number 5";
        10: print "This is the number 10";
        n: print "This is the number: " + n; 
    }
```

You can add conditonals with the **if** keyword to patterns to add more complex conditions, like in the FizzBuzz example:

```
// FizzBuzz in LoxCpp
for i in 1..20
    match i {
        n if n % 15 == 0: print "FizzBuzz";
        n if n % 5 == 0: print "Buzz";
        n if n % 3 == 0: print "Fizz";
        n: print n;
    }
```

## Natives

There are a bunch of natives or built-in functions in LoxCpp. Without entering in too much detail, since they are subjet to change at this point, they are the following:

### Basic
**clock:** returns the current value of the clock, good for performance measuring.
**sizeOf:** returns the size of an object.

### Types
**isList:** returns if a value is a list.
**inBounds:** returns if a value is within the bounds of a list or range.

### IO
**readInput:** reads the user input.
**readFile:** returns the content of a file.
**writeFile:** writes a string to a file.

### Lists
**push:** pushes a value to the back of a list.
**pop:** removes the value at the back of a list and returns it.
**erase:** removes a value of a list given an index.
**concat:** concatenates two lists.

### Iterables
Iterables are lists, ranges and strings.

**contains:** checks if an iterable contains a value.
**indexOf:** fins a value on a an iterable and returns its index, or nil.
**findIf:** finds a value on an iterable given a function. Returns the value or nil.
**map:** standar map function.
**filter:** standard filter function.
**reduce:** standard reduce function.
