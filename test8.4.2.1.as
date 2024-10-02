	lw	0	1	posOne
	lw	0	2	a
	lw	0	3	b
loop	beq	2	3	end
	nor	2	2	4
	add	4	1	4
	add	4	3	5
	lw	0	6	mask
	nor	5	5	5
	nor	6	6	6
	nor	5	6	6
	beq	0	6	less
	nor	3	3	4
	add	4	1	4
	add	2	4	2
	beq	0	0	loop
less	add	3	4	3
	beq	0	0	loop
end	sw	0	2	result
	halt
posOne	.fill	1
mask	.fill	-32768
a	.fill	18
b	.fill	12
result	.fill	1
