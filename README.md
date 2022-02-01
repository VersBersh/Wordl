# Wordl Bot

A small bot to beat the game wordl.

## Requirements

- python >= 3
- _(optional)_ a c++ compiler (tested on clang++, but others should work)

## Set up / Usage

First clone this repo

```sh
 git clone git@github.com:VersBersh/Wordl.git ./wordl
```

Game trees for some words are already in the `solutions` directory. You can use these already with the `Game` class in a python/IPython console

```py
import wordl

wordl.Game(first_guess="salet").play()
```

note: this is not a proper Python package. You'll just need to launch the python console from this directory.

If you want to have solutions for other words you need to build the solver.

If you're using `clang++` then you can build the solver by following the recipe in the Makefile

```sh
cd src
nmake.exe -f ./Makefile.win build
```

once you have an executable in `./bin` then you can run the solver

```py
import wordl

wordl.optimise("trace")
```


## Results

<table border="1px solid back" cellspacing="0">
    <tr>
        <th rowspan=2> Word </th>
        <th rowspan=2> Average Depth </th>
        <th rowspan=2> Max Depth </th>
        <th rowspan=2> Total Guesses </th>
        <th rowspan=2> Execution Time </th>
        <th colspan=3> Hyperparameters </th>
    </tr>
    <tr>
        <th> top_n </th>
        <th> max_n </th>
        <th> max_tree_depth </th>
    </tr>
    <tr>
        <td>salet</td> <td>3.42117</td> <td>5</td> <td>7920</td> <td>0m 37s</td> <td>6</td> <td>50</td> <td>7</td>
    </tr>
    <tr>
        <td>reast</td> <td>3.42246</td> <td>6</td> <td>7923</td> <td>0m 41s</td> <td>6</td> <td>50</td> <td>7</td>
    <tr>
    </tr>
        <td>crate</td> <td>3.42419</td> <td>6</td> <td>7927</td> <td>0m 47s</td> <td>6</td> <td>50</td> <td>7</td>
    </tr>
    </tr>
        <td>arose</td> <td>3.46048</td> <td>6</td> <td>8011</td> <td>1m 31s</td> <td>6</td> <td>50</td> <td>7</td>
    </tr>
</table>
