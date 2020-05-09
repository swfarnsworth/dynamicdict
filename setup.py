from distutils.core import setup, Extension


with open('./README.md') as f:
    long_description = f.read()


def main():
    setup(
        name='dynamicdict',
        version='0.0.1',
        description='Dict-like type that creates values for missing keys in terms of the ke',
        long_description=long_description,
        author='Steele Farnsworth',
        author_email='swfarnsworth@gmail.com',
        url='https://github.com/swfarnsworth/dynamicdict',
        ext_modules=[Extension('dynamicdict', ['dynamicdict.c'])]
    )


if __name__ == '__main__':
    main()
