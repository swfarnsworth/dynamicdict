# dynamicdict

`dynamicdic` is a Python dict-like class that mirrors the functionality of `defaultdict` in the `collections` library 
of the Python standard library. Similar to `defaultdict`, `dynamicdict` calls a `default_factory` function
(or any other callable) when one attempts to access a value for which the key does not exist in the dictionary.
`dynamicdict`, however, passes the missing key to this function. Thus the values for missing keys are created
dynamically based on the value of the key itself.

```pythonstub
>>> from dynamicdict import dynamicdict
>>> my_dyd = dynamicdict(str.upper)
>>> my_dyd['hello'] += ' world'
'HELLO world'

>>> class Person:
...     def __init__(self, name, age=0):
...         self.name = name
...         self.age = age
...     def have_birthday(self):
...         self.age += 1
...         print(f"{self.name} has turned {self.age}")
>>> people_dyd = dynamicdict(Person)
>>> people_dyd['Bob'] = Person('Bob', 42)
>>> for friend in ['Bob', 'Alice']:
...     people_dyd[friend].have_birthday()
'Bob has turned 43'
'Alice has turned 1'
```

Callables that require more than one positional argument or keyword arguments can be created with lambdas,
provided that all other arguments can be constants or themselves functions of the missing key.

```python
>>> dyd = dynamicdict(lambda x: Person(x, 10))
>>> dyd['Carl'].have_birthday()
'Carl has turned 11'
``` 

Instantiating a `dynamicdict` with a callable that requires more than one argument will raise
`TypeError` when one attempts to access a missing key.

```python
>>> dyd = dynamicdict(lambda x, y: x + y)
>>> dyd[2] += 3
TypeError: <lambda>() missing 1 required positional argument: 'y'
```

This class is implemented in C, though it is equivalent to the following pure Python code:

```python
from collections import defaultdict

class dynamicdict(defaultdict):

    def __missing__(self, key):
        new_val = self[key] = self.default_factory(key)
        return new_val
```

## Installation

This module can be installed from github
```bash
pip install git+https://github.com/swfarnsworth/dynamicdict.git
```

Note that GCC is required to compile this module, but the compilation is handled automatically.
