from distutils.core import setup
from distutils.extension import Extension
#from distutils.sysconfig import get_python_inc
from Cython.Distutils import build_ext
from Cython.Build import cythonize
import numpy

numpy_include = numpy.get_include()

extensions = [
    Extension(
        'exmodule.graph',
        sources = ['exmodule/graph.pyx'],
        include_dirs = [numpy_include],
        language = 'c++',
        extra_compile_args = ['-std=c++11']
    )
]

setup(
    name = 'CythonExamples',
    author = 'Kentaro Minami',
    description = 'Some examples of cython applications',
    packages = ['exmodule'],
    ext_modules = cythonize(extensions),
    cmdclass = { 'build_ext': build_ext }
)
