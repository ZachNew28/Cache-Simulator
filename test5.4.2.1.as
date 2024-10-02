	lw	0	1	One
	lw	0	2	Len
	lw	0	3	Zero
loop	beq	3	2	end
	lw	3	4	Arr
	beq	4	1	cont
	add	5	1	5
cont	add	3	1	3
	beq	0	0	loop
end	halt
Zero	.fill	0
One	.fill	1
Len	.fill	3
Arr	.fill	1
	.fill	1
	.fill	0
