#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_ADD_INCLUDEDIRS := .

CFLAGS += -Wno-error=sequence-point -Wno-error=parentheses -Wno-error=unused-value

