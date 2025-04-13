# Catalox

Joke(ish) version of [Loxcpp](https://github.com/XDargu/loxcpp) called **Catalox**, that implements [Loxcpp](https://github.com/XDargu/loxcpp) with keywords and function names 100% in [Catalan](https://en.wikipedia.org/wiki/Catalan_language).

The original [Loxcpp](https://github.com/XDargu/loxcpp) is a simple C++ implementation of the Lox programming language from [Crafting Interpreters](https://craftinginterpreters.com/) with a few extra features.

Catalox is a dynamically typed, garbage collected language. It supports both functional patterns and the basics of object orientation.
Catalox differs from Lox in some areas and have several extra features, since I wanted to take the language in a different direction. The focus is still on a simple syntax, easy to learn and understand at a glance.

Let's start with the basics. This is how you do a simple Hello World in Catalox:

```
// Hello World in Catalox
imprimeix "Hello, world!";
```

Moving on to something a bit more complex, this is a simple implementation of the FizzBuzz program in Catalox:

```
// FizzBuzz in Catalox
per num a 1..20
    partit num {
        n si n % 15 == 0: imprimeix "FizzBuzz";
        n si n % 5 == 0: imprimeix "Buzz";
        n si n % 3 == 0: imprimeix "Fizz";
        n: imprimeix n;
    }
```

Let's look at a classic recursion example, calculating the Nth fibonacci number:

```
proces fib(n)
{
    si (n < 2) torna n;
    sino torna fib(n-1) + fib(n-2);
}
```

# Basic keywords

Since Catalox uses exclusively Catalan names, the familiar keywords (if, else, class, and, return...) are all different. Let's start with a list of all the LoxCpp keywords, and how they translate in Catalox:

- and: **i**
- break: **trenca**
- case: **cas**
- class: **classe**
- const: **fix**
- continue: **segueix**
- else: **sino**
- if: **si**
- in: **a**
- nil: **res**
- match: **partit**
- or: **o**
- print: **imprimeix**
- return: **torna**
- super: **super/pare**
- var: **fes**
- while: **mentre**
- false: **fals**
- for: **per**
- fun: **proces**
- this: **aixo**
- true: **veritable**

This means that a simple function that returns two numbers added together is the following in Catalox:

```
proces add(a, b) { torna a + b; }
```

This is an example (and overly verbose) function that returns if both input numbers are even:

```
proces bothEvent(a, b)
{
    si ((a % 2 == 0) i (b % 2 == 0))
        torna veritable;
    sino
        torna fals;
}
```

Finally, this is a simple class delcaration:

```
classe Student
{
    // init is the construction method
    init(name, surname, id)
    {
        aixo.name = name;
        aixo.surname = surname;
        aixo.id = id;
        aixo.isGraduated = fals;
    }

    graduate()
    {
        aixo.isGraduated = true;
    }

    getFullName()
    {
        torna aixo.name + " " + aixo.surname;
    }
}

// Create a new student
fes student = Student("Jaume", "Serra", 2394);

// Print the student name
imprimeix student.getFullName();
```

Let't now look at all the extra features that [Loxcpp](https://github.com/XDargu/loxcpp) has, and how they work in Catalox. All of them are, of course, direct translations from Loxcpp.

# Fix (Const) variables
Catalox has two different ways of declaring variables: **fes** and **fix**. Fix, doesn't allow changes to the value after its declaration.

```
// Compiles correctly
fes a = 10;
a = 2;

// Compilation error: "Can't reassign a const variable"
fix b = 10;
b = 2;
```

## Lists
Catalox supports basic lists. A list is simply an ordered collection of elements. Lists in Catalox are not strongly typed, which means you can add pretty much anything you want to a list:

```
fix list = [1, 50, "Hello World", nil, 256];
```

You can access list elements using square brackets, as it's common in many other languages:

```
fes list = [1, 50, "Hello World", nil, 256];

// Prints 1
imprimeix list[0]

// Prints "Hello World"
imprimeix list[2]

list[2] = 20;
// Prints 20
imprimeix list[2];
```

You can add or remove values with the **afegir** (push) and **treure** (pop) operations:

```
fes list = [];

afegir(list, 5);
afegir(list, 10);
// Prints [5, 10]
imprimeix list;

fix value = treure(list);
// Prints 10
imprimeix value;
// Prints [5]
imprimeix list;
```

## Per-a (For-In)

On top of the basic loops, Catalox adds the per-a (for-in) loop, which allows iterating over an iterable type, and performing operations on each value. This works for lists, ranges or strings.

```
fix name = "Daniel";

// Prints "D" "a" "n" "i" "e" "l" on different lines
per c a name
  imprimeix c;
```

```
// Prints 5 6 7 8 on different lines
per c a [5, 6, 7, 8]
  imprimeix c;
```

## Ranges
Ranges are a neat type that allows representation of a range of values without actually having to keep them in memory, like in the case of lists.

For example, if you want to make a loop that interates on all numbers between 1 and 100, you could simply do it by using a range. Unlike a list, the range will not allocate memory for each value, it will simply use an iterator to visit all values, one at a time.

**Note:** Due to the name **i** being a keyword in Catalox (*i* means *and* in Catalan), you can't use it as the default for loop variable name. Sorry!

```
per n a 1..100
  imprimeix n;
```

Ranges can go in both directions: increasing or decreasing values:

```
per n a 100..1
  imprimeix i;
```

## Anonymous functions
Anonymous functions or "lambda functions" allow the creation of functions without giving them a name or assigning them to a variable.

They are commonly used when calling higher-order functions.

Catalox supports anonymous functions with the same syntax as normal function declaration, using the keyworkd **proces**. Like in [Loxcpp](https://github.com/XDargu/loxcpp), one of the key ideas of Catalox is ensure there are not several ways of doing essentially the same thing (ahem C++...).

This is a simple example of getting the even numbers from a list with both a named and an annonymous function:

```
fix values = [1, 50, 77, 256];

// Using an annonymous function
fix evenNumbers = filtra(values, proces(num){ torna num % 2 == 0; });

// Using a named function
proces isEven(num) { torna num % 2 == 0; };
fix evenNumbers2 = filtra(values, isEven);

// Both print [50, 256]
imprimeix evenNumbers;
imprimeix evenNumbers2;
```

## Pattern matching

Catalox has basic pattern matching by using the **partit** (match) keyword. You can match a value to any value. If the values are equal, the code on the right of the pattern will be executed:

```
fix value = 5;
partit value {
        5: imprimeix "This is the number 5";
        10: imprimeix "This is the number 10";
        "Hello World": imprimeix "This is the string Hello World";
    }
```

There are a few neat things you can do with pattern matching.

You can match ranges:

```
partit(value)
{
    1..10: imprimeix "Value from 1 to 10";
    20: imprimeix "That's a 20";
}
```

You can capture the value you are matching, to then use it later, by declaring a named variable as the pattern:

```
fix value = 5;
partit value {
        5: imprimeix "This is the number 5";
        10: imprimeix "This is the number 10";
        n: imprimeix "This is the number: " + n; 
    }
```

You can add conditonals with the **si** (if) keyword to patterns to add more complex conditions, like in the FizzBuzz example:

```
// FizzBuzz in Catalox
per num a 1..20
    partit num {
        n si n % 15 == 0: imprimeix "FizzBuzz";
        n si n % 5 == 0: imprimeix "Buzz";
        n si n % 3 == 0: imprimeix "Fizz";
        n: imprimeix n;
    }
```

## Natives

There are a bunch of natives or built-in functions in [Loxcpp](https://github.com/XDargu/loxcpp). Without entering in too much detail, since they are subjet to change at this point.
They are taken directly from [Loxcpp](https://github.com/XDargu/loxcpp). I will leave the original name of the functions as reference:

### Basic
- **rellotge (clock):** returns the current value of the clock, good for performance measuring.
- **mida (sizeOf):** returns the size of an object.

### Types
- **esLlista (isList):** returns if a value is a list.
- **enLimits (inBounds):** returns if a value is within the bounds of a list or range.

### IO
- **llegirTeclat (readInput):** reads the user input.
- **llegirFitxer (readFile):** returns the content of a file.
- **escriureFitxer (writeFile):** writes a string to a file.

### Lists
- **afegir (push):** pushes a value to the back of a list.
- **treure (pop):** removes the value at the back of a list and returns it.
- **esborrar (erase):** removes a value of a list given an index.
- **ajuntar (concat):** concatenates two lists.

### Iterables
Iterables are lists, ranges and strings.

- **conte (contains):** checks if an iterable contains a value.
- **indexDe (indexOf):** fins a value on a an iterable and returns its index, or nil.
- **trobaSi (findIf):** finds a value on an iterable given a function. Returns the value or nil.
- **mapa (map):** standar map function.
- **filtra (filter):** standard filter function.
- **redueix (reduce):** standard reduce function.
