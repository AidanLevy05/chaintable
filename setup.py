from setuptools import setup, Extension

setup(
    name="hashmod",
    ext_modules=[Extension(
        "hashmod",
        sources=["src/hashtable.c", "src/hashmodule.c"],
        include_dirs=["include"],
    )],
)