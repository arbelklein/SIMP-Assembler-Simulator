#binom.asm

Main:
	sll $sp, $imm1, $imm2, $zero, 1, 11				# Setting $sp = 1 << 11 = 2048
	lw $a0, $imm1, $zero, $zero, 256, 0				# Loading n from 0x100 (256) address to $a0
	lw $a1, $imm1, $zero, $zero, 257, 0				# Loading k from 0x101 (257) address to $a1
	jal $ra, $zero, $zero, $imm2, 0, Binom			# Calculating $v0 = binom(n, k)
	sw $v0, $imm1, $zero, $zero, 258, 0				# Storing the return value ($v0) in 0x102 (258) address
	halt $zero, $zero, $zero, $zero, 0, 0			# Halt execution, exit simulator

Base:
	add $v0, $imm1, $zero, $zero, 1, 0				# Setting return value ($v0) to 1
	beq $zero, $zero, $zero, $imm2, 0, Exit			# Jumping to Exit	

	
Binom:
	# Managing the stack
	sub $sp, $sp, $imm1, $zero, 4, 0				# Allocating space in the stack for 4 items
	sw $zero, $sp, $imm1, $s0, 0, 0					# Storing $s0 in the stack
	sw $zero, $sp, $imm1, $ra, 1, 0					# Storing $ra in the stack				
	sw $zero, $sp, $imm1, $a0, 2, 0					# Storing $a0 in the stack
	sw $zero, $sp, $imm1, $a1, 3, 0					# Storing $a1 in the stack
	
	# The function
	beq $zero, $a1, $zero, $imm2, 0, Base			# Jumping to Base if k = 0
	sub $t0, $a0, $a1, $zero, 0, 0					# $t0 = n - k
	beq $zero, $t0, $zero, $imm2, 0, Base			# Jumping to Base if n - k = 0, aka if n = k

	# Recursive part
	sub $a0, $a0, $imm1, $zero, 1, 0				# Setting n = n -1
	jal $ra, $zero, $zero, $imm2, 0, Binom			# Calculating $v0 = binom(n - 1, k)
	add $s0, $v0, $zero, $zero, 0, 0				# Setting $s0 = $v0
	sub $a1, $a1, $imm1, $zero, 1, 0				# Setting k = k - 1
	jal $ra, $zero, $zero, $imm2, 0, Binom			# Calculating $v0 = binom(n - 1, k - 1)
	add $v0, $v0, $s0, $zero, 0, 0					# Setting $v0 = $v0 + $s0, aka $v0 = binom(n - 1, k - 1) + binom(n - 1, k)
	beq $zero, $zero, $zero, $imm2, 0, Exit			# Jumping to Exit	

		
Exit:
	# Managing the stack
	lw $s0, $sp, $imm1, $zero, 0, 0					# Restoring $s0 from stack
	lw $ra, $sp, $imm1, $zero, 1, 0					# Restoring $ra from stack
	lw $a0, $sp, $imm1, $zero, 2, 0					# Restoring $a0 from stack
	lw $a1, $sp, $imm1, $zero, 3, 0					# Restoring $a1 from stack
	add $sp, $sp, $imm1, $zero, 4, 0				# Freeing space in the stack
	
	# Exiting
	beq $zero, $zero, $zero, $ra, 0, 0				# Returning to the caller