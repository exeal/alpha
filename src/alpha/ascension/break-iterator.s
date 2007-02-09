	.file	"break-iterator.cpp"
	.text
	.align 2
	.def	__ZSt17__verify_groupingPKcjRKSs;	.scl	3;	.type	32;	.endef
__ZSt17__verify_groupingPKcjRKSs:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	16(%ebp), %eax
	movl	%eax, (%esp)
	call	__ZNKSs4sizeEv
	decl	%eax
	movl	%eax, -4(%ebp)
	movl	12(%ebp), %eax
	decl	%eax
	movl	%eax, -12(%ebp)
	leal	-12(%ebp), %eax
	movl	%eax, 4(%esp)
	leal	-4(%ebp), %eax
	movl	%eax, (%esp)
	call	__ZSt3minIjERKT_S2_S2_
	movl	(%eax), %eax
	movl	%eax, -8(%ebp)
	movl	-4(%ebp), %eax
	movl	%eax, -16(%ebp)
	movb	$1, -17(%ebp)
	movl	$0, -24(%ebp)
L2:
	movl	-24(%ebp), %eax
	cmpl	-8(%ebp), %eax
	jae	L5
	cmpb	$0, -17(%ebp)
	je	L5
	movl	-16(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	16(%ebp), %eax
	movl	%eax, (%esp)
	call	__ZNKSsixEj
	movl	%eax, %ecx
	movl	-24(%ebp), %eax
	movl	8(%ebp), %edx
	addl	%eax, %edx
	movzbl	(%ecx), %eax
	cmpb	(%edx), %al
	sete	%al
	movb	%al, -17(%ebp)
	leal	-16(%ebp), %eax
	decl	(%eax)
	leal	-24(%ebp), %eax
	incl	(%eax)
	jmp	L2
L5:
	cmpl	$0, -16(%ebp)
	je	L6
	cmpb	$0, -17(%ebp)
	je	L6
	movl	-16(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	16(%ebp), %eax
	movl	%eax, (%esp)
	call	__ZNKSsixEj
	movl	%eax, %ecx
	movl	-8(%ebp), %eax
	movl	8(%ebp), %edx
	addl	%eax, %edx
	movzbl	(%ecx), %eax
	cmpb	(%edx), %al
	sete	%al
	movb	%al, -17(%ebp)
	leal	-16(%ebp), %eax
	decl	(%eax)
	jmp	L5
L6:
	movl	$0, 4(%esp)
	movl	16(%ebp), %eax
	movl	%eax, (%esp)
	call	__ZNKSsixEj
	movl	%eax, %ecx
	movl	-8(%ebp), %eax
	movl	8(%ebp), %edx
	addl	%eax, %edx
	movzbl	(%ecx), %eax
	cmpb	(%edx), %al
	jg	L8
	movzbl	-17(%ebp), %eax
	andl	$1, %eax
	movb	%al, -25(%ebp)
	jmp	L9
L8:
	movb	$0, -25(%ebp)
L9:
	movzbl	-25(%ebp), %eax
	movb	%al, -17(%ebp)
	movzbl	-17(%ebp), %eax
	leave
	ret
.globl __ZTVN9ascension7unicode29AbstractSentenceBreakIteratorE
	.section	.rdata$_ZTVN9ascension7unicode29AbstractSentenceBreakIteratorE,"dr"
	.linkonce same_size
	.align 32
__ZTVN9ascension7unicode29AbstractSentenceBreakIteratorE:
	.long	0
	.long	__ZTIN9ascension7unicode29AbstractSentenceBreakIteratorE
	.long	__ZN9ascension7unicode29AbstractSentenceBreakIteratorD1Ev
	.long	__ZN9ascension7unicode29AbstractSentenceBreakIteratorD0Ev
	.long	__ZNK9ascension7unicode29AbstractSentenceBreakIterator10isBoundaryERKNS0_17CharacterIteratorE
	.long	__ZN9ascension7unicode29AbstractSentenceBreakIterator4nextEi
	.long	___cxa_pure_virtual
	.long	___cxa_pure_virtual
.globl __ZTVN9ascension7unicode25AbstractWordBreakIteratorE
	.section	.rdata$_ZTVN9ascension7unicode25AbstractWordBreakIteratorE,"dr"
	.linkonce same_size
	.align 32
__ZTVN9ascension7unicode25AbstractWordBreakIteratorE:
	.long	0
	.long	__ZTIN9ascension7unicode25AbstractWordBreakIteratorE
	.long	__ZN9ascension7unicode25AbstractWordBreakIteratorD1Ev
	.long	__ZN9ascension7unicode25AbstractWordBreakIteratorD0Ev
	.long	__ZNK9ascension7unicode25AbstractWordBreakIterator10isBoundaryERKNS0_17CharacterIteratorE
	.long	__ZN9ascension7unicode25AbstractWordBreakIterator4nextEi
	.long	___cxa_pure_virtual
	.long	___cxa_pure_virtual
.globl __ZTVN9ascension7unicode29AbstractGraphemeBreakIteratorE
	.section	.rdata$_ZTVN9ascension7unicode29AbstractGraphemeBreakIteratorE,"dr"
	.linkonce same_size
	.align 32
__ZTVN9ascension7unicode29AbstractGraphemeBreakIteratorE:
	.long	0
	.long	__ZTIN9ascension7unicode29AbstractGraphemeBreakIteratorE
	.long	__ZN9ascension7unicode29AbstractGraphemeBreakIteratorD1Ev
	.long	__ZN9ascension7unicode29AbstractGraphemeBreakIteratorD0Ev
	.long	__ZNK9ascension7unicode29AbstractGraphemeBreakIterator10isBoundaryERKNS0_17CharacterIteratorE
	.long	__ZN9ascension7unicode29AbstractGraphemeBreakIterator4nextEi
	.long	___cxa_pure_virtual
	.long	___cxa_pure_virtual
.globl __ZNSt5ctypeIwE2idE
	.section	.data$_ZNSt5ctypeIwE2idE,"w"
	.linkonce same_size
	.align 4
__ZNSt5ctypeIwE2idE:
	.space 4
.globl __ZGVNSt5ctypeIwE2idE
	.section	.data$_ZGVNSt5ctypeIwE2idE,"w"
	.linkonce same_size
	.align 8
__ZGVNSt5ctypeIwE2idE:
	.space 8
.globl __ZTIN9ascension7unicode29AbstractGraphemeBreakIteratorE
	.section	.rdata$_ZTIN9ascension7unicode29AbstractGraphemeBreakIteratorE,"dr"
	.linkonce same_size
	.align 4
__ZTIN9ascension7unicode29AbstractGraphemeBreakIteratorE:
	.long	__ZTVN10__cxxabiv120__si_class_type_infoE+8
	.long	__ZTSN9ascension7unicode29AbstractGraphemeBreakIteratorE
	.long	__ZTIN9ascension7unicode13BreakIteratorE
.globl __ZTIN9ascension7unicode25AbstractWordBreakIteratorE
	.section	.rdata$_ZTIN9ascension7unicode25AbstractWordBreakIteratorE,"dr"
	.linkonce same_size
	.align 4
__ZTIN9ascension7unicode25AbstractWordBreakIteratorE:
	.long	__ZTVN10__cxxabiv120__si_class_type_infoE+8
	.long	__ZTSN9ascension7unicode25AbstractWordBreakIteratorE
	.long	__ZTIN9ascension7unicode13BreakIteratorE
.globl __ZTIN9ascension7unicode29AbstractSentenceBreakIteratorE
	.section	.rdata$_ZTIN9ascension7unicode29AbstractSentenceBreakIteratorE,"dr"
	.linkonce same_size
	.align 4
__ZTIN9ascension7unicode29AbstractSentenceBreakIteratorE:
	.long	__ZTVN10__cxxabiv120__si_class_type_infoE+8
	.long	__ZTSN9ascension7unicode29AbstractSentenceBreakIteratorE
	.long	__ZTIN9ascension7unicode13BreakIteratorE
.globl __ZTSN9ascension7unicode29AbstractGraphemeBreakIteratorE
	.section	.rdata$_ZTSN9ascension7unicode29AbstractGraphemeBreakIteratorE,"dr"
	.linkonce same_size
	.align 32
__ZTSN9ascension7unicode29AbstractGraphemeBreakIteratorE:
	.ascii "N9ascension7unicode29AbstractGraphemeBreakIteratorE\0"
.globl __ZTSN9ascension7unicode25AbstractWordBreakIteratorE
	.section	.rdata$_ZTSN9ascension7unicode25AbstractWordBreakIteratorE,"dr"
	.linkonce same_size
	.align 32
__ZTSN9ascension7unicode25AbstractWordBreakIteratorE:
	.ascii "N9ascension7unicode25AbstractWordBreakIteratorE\0"
.globl __ZTSN9ascension7unicode29AbstractSentenceBreakIteratorE
	.section	.rdata$_ZTSN9ascension7unicode29AbstractSentenceBreakIteratorE,"dr"
	.linkonce same_size
	.align 32
__ZTSN9ascension7unicode29AbstractSentenceBreakIteratorE:
	.ascii "N9ascension7unicode29AbstractSentenceBreakIteratorE\0"
	.section .rdata,"dr"
	.align 4
__ZN9ascension13INVALID_INDEXE:
	.long	-1
	.align 2
__ZN9ascension9LINE_FEEDE:
	.word	10
	.align 2
__ZN9ascension15CARRIAGE_RETURNE:
	.word	13
	.align 2
__ZN9ascension9NEXT_LINEE:
	.word	133
	.align 2
__ZN9ascension21ZERO_WIDTH_NON_JOINERE:
	.word	8204
	.align 2
__ZN9ascension17ZERO_WIDTH_JOINERE:
	.word	8205
	.align 2
__ZN9ascension14LINE_SEPARATORE:
	.word	8232
	.align 2
__ZN9ascension19PARAGRAPH_SEPARATORE:
	.word	8233
	.align 2
__ZN9ascension12NONCHARACTERE:
	.word	-1
	.align 4
__ZN9ascension18INVALID_CODE_POINTE:
	.long	-1
	.align 2
__ZN9ascension21LINE_BREAK_CHARACTERSE:
	.word	10
	.word	13
	.word	133
	.word	8232
	.word	8233
	.align 4
__ZN9ascension7unicode12NOT_PROPERTYE:
	.long	0
.globl __ZTIN9ascension7unicode13BreakIteratorE
	.section	.rdata$_ZTIN9ascension7unicode13BreakIteratorE,"dr"
	.linkonce same_size
	.align 4
__ZTIN9ascension7unicode13BreakIteratorE:
	.long	__ZTVN10__cxxabiv117__class_type_infoE+8
	.long	__ZTSN9ascension7unicode13BreakIteratorE
.globl __ZTSN9ascension7unicode13BreakIteratorE
	.section	.rdata$_ZTSN9ascension7unicode13BreakIteratorE,"dr"
	.linkonce same_size
	.align 32
__ZTSN9ascension7unicode13BreakIteratorE:
	.ascii "N9ascension7unicode13BreakIteratorE\0"
	.section	.ctors,"w"
	.align 4
	.long	__GLOBAL__I__ZN9ascension7unicode29AbstractGraphemeBreakIteratorC2ERKSt6locale
	.def	__ZN9ascension7unicode29AbstractGraphemeBreakIteratorD0Ev;	.scl	3;	.type	32;	.endef
	.def	__ZN9ascension7unicode29AbstractGraphemeBreakIteratorD1Ev;	.scl	3;	.type	32;	.endef
	.def	__ZN9ascension7unicode25AbstractWordBreakIteratorD0Ev;	.scl	3;	.type	32;	.endef
	.def	__ZN9ascension7unicode25AbstractWordBreakIteratorD1Ev;	.scl	3;	.type	32;	.endef
	.def	___cxa_pure_virtual;	.scl	3;	.type	32;	.endef
	.def	__ZN9ascension7unicode29AbstractSentenceBreakIteratorD0Ev;	.scl	3;	.type	32;	.endef
	.def	__ZN9ascension7unicode29AbstractSentenceBreakIteratorD1Ev;	.scl	3;	.type	32;	.endef
	.def	__ZNKSsixEj;	.scl	3;	.type	32;	.endef
	.def	__ZNKSs4sizeEv;	.scl	3;	.type	32;	.endef
	.def	__ZSt3minIjERKT_S2_S2_;	.scl	3;	.type	32;	.endef
