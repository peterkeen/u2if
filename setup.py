from setuptools import setup, find_packages

setup(
    name='u2if',
    version='0.6',
    description='Micropython Machine module for x86 with pico bridge',
    url='http://github.com/execuc/u2if',
    author='execuc',
    author_email='',
    license='MIT',
    package_dir={"": "source"},
    packages=find_packages(where="source"),
    zip_safe=False,
    install_requires=[
        "pyserial>=3.5",
        "hid>=1.0.4",
        "micropython-cpython-ustruct==0.0",
        "micropython-cpython-micropython==0.1.1",
    ],
)
