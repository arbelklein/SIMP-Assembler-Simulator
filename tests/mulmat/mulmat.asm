	add $s0, $imm2, $zero, $zero, 0, 0x100	# $s0=A
	add $s1, $imm2, $zero, $zero, 0, 0x110	# $s1=B
	add $s2, $imm2, $zero, $zero, 0, 0x120	# $s2=C
	add $a0, $zero, $zero, $zero, 0, 0	# $a0=row=0
TEST_1:
	blt $zero, $a0, $imm1, $imm2, 4, LOOP_1		# if row<4 then go to LOOP_1
	beq $zero, $zero, $zero, $imm2, 0, EXIT	# otherwise EXIT
LOOP_1:
	add $a1, $zero, $zero, $zero, 0, 0	# $a1=col=0
	beq $zero, $zero, $zero, $imm2, 0, TEST_2 # go to TEST_2
TEST_2:
	blt $zero, $a1, $imm1, $imm2, 4, LOOP_2		# if col<4 then go to LOOP_2
	add $a0, $a0, $imm1, $zero, 1, 0	# row++
	beq $zero, $zero, $zero, $imm2, 0, TEST_1 # go to TEST_1
LOOP_2:
	add $v0, $zero, $zero, $zero, 0, 0	# $v0=0
	add $a2, $zero, $zero, $zero, 0, 0	# $a2=0
	jal $ra, $zero, $zero, $imm2, 0, dot_product	# saves next code line and calculate dot prodact to $v0
	sw $v0, $s2, $zero, $zero, 0, 0 # C[$s2]=$v0 = result of dot product: A[row]*(B[col]^t)
	add $s2, $s2, $imm1, $zero, 1, 0 # go to next cell of C matrix for next iteration
	add $a1, $a1, $imm1, $zero, 1, 0	# col++
	beq $zero, $zero, $zero, $imm2, 0, TEST_2 # go to TEST_2
dot_product:
	mac $t0, $a0, $imm1, $a2, 4, 0 # $t0 = $row*4 + $a2
	add $t0, $t0, $s0, $zero, 0, 0	# $t0=A + ($row*4 + $a2)
	lw $t0, $zero, $t0, $zero, 0, 0	# $t0=A[$row*4 + $a2]
	mac $t1, $a2, $imm1, $a1, 4, 0	# $t1 = $a2*4 + col
	add $t1, $t1, $s1, $zero, 0, 0	# $t1=B + ($a2*4 + col)
	lw $t1, $zero, $t1, $zero, 0, 0	# $t1=B[$a2*4 + col]
	mac $v0, $t0, $t1, $v0, 0, 0	# $v0=$v0+A[$row*4 + $a2]*B[$a2*4 + col]
	add $a2, $a2, $imm1, $zero, 1, 0	# a2++
	blt $zero, $a2, $imm1, $imm2, 4, dot_product	# if $a2<4 then go to next iteration of dot_product:
	beq $zero, $zero, $zero, $ra, 0, 0	# otherwise go back to where we stoped inside LOOP_2
EXIT:
	halt $zero, $zero, $zero, $zero, 0, 0	# halt