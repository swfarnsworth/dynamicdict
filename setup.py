from distutils.core import setup, Extension


def main():
    setup(name="dynamicdict",
          version="1.0.0",
          description="TODO",
          author="Steele Farnsworth",
          author_email="swfarnsworth@gmail.com",
          ext_modules=[Extension("dynamicdict", ["./dynamicdict/dynamicdict.c"])]
    )


if __name__ == "__main__":
    main()
