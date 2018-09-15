packagedir=$(pkglibdir)/java/classes/org/eyedb/syscls
dist_package_JAVA =				\
AgregatClassComponent.java			\
AgregatClassExecutable.java			\
ArgType.java					\
ArgType_Type.java				\
AttributeComponent.java				\
AttributeComponentSet.java			\
AttributeConvertType.java			\
BEMethod_C.java					\
BEMethod.java					\
BEMethod_OQL.java				\
BTreeIndex.java					\
CardinalityConstraint.java			\
CardinalityConstraint_Test.java			\
CardinalityDescription.java			\
ClassComponent.java				\
ClassConversion.java				\
ClassUpdateType.java				\
ClassVariable.java				\
CollAttrImpl.java				\
Database.java					\
Executable.java					\
ExecutableLang.java				\
ExecutableLocalisation.java			\
ExecutableType.java				\
FEMethod_C.java					\
FEMethod.java					\
HashIndex.java					\
Index.java					\
IndexType.java					\
Method.java					\
NotNullConstraint.java				\
Protection.java					\
ProtectionMode.java				\
ProtectionUser.java				\
Root.java					\
set_class_AttributeComponent_ref.java		\
Signature.java					\
Trigger.java					\
TriggerType.java				\
UniqueConstraint.java				\
UnreadableObject.java

JAVAROOT=$(top_builddir)/src/java/src
CLASSPATH_ENV= CLASSPATH=$(JAVAROOT):$(top_srcdir)/src/java/src

EYEDBODL_exe=$(bindir)/eyedbodl

EYEDBODL = $(EYEDBODL_exe) --gencode=java --output-dir=$(top_srcdir)/src/java/src/org/eyedb/syscls

EYEDBODLFLAGS_syscls = --export --package=org.eyedb.syscls \
        --schema-name=eyedb --attr-style=explicit --down-casting=yes \
        --class-enums=yes --system-class \
        --error-policy=exception

JAVA_ODL_FILES= \
$(top_srcdir)/src/java/src/org/eyedb/syscls/Database.java 

$(JAVA_ODL_FILES): $(top_srcdir)/src/eyedb/syscls.odl
	@if test -x $(EYEDBODL_exe) ; then \
		echo $(EYEDBODL) $(EYEDBODLFLAGS_syscls) $< ; $(EYEDBODL) $(EYEDBODLFLAGS_syscls) $< ; \
	else \
		echo Warning: $(EYEDBODL) not found, using existing $@; touch $@ ; \
	fi


