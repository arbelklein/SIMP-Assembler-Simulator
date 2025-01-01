#disktest.asm

Main:
	# Manging the stack
	sll $sp, $imm1, $imm2, $zero, 1, 11				# Setting $sp = 1 << 11 = 2048
	sub $sp, $sp, $imm1, $zero, 4, 0				# Allocating space in the stack for 2 items
	sw $zero, $sp, $imm1, $s0, 0, 0					# Storing $s0 in the stack
	sw $zero, $sp, $imm1, $s1, 1, 0					# Storing $s1 in the stack
	sw $zero, $sp, $imm1, $s2, 2, 0					# Storing $s2 in the stack
	sw $zero, $sp, $imm1, $ra, 3, 0					# Storing $ra in the stack
	
	add $s0, $zero, $imm1, $zero, 1000, 0			# $s0 = 1000, will be used for diskbuffer
	add $s1, $zero, $imm1, $zero, 7, 0				# $s1 = 7, will be used for the sector to read from
	add $s2, $zero, $imm1, $zero, 8, 0				# $s2 = 8, will be used for the sector to write to
	
	beq $zero, $zero, $zero, $imm2, 0, Loop1		# Jumping to Loop1
	
Free:
	in $t0, $imm1, $zero, $zero, 17, 0				# Reading the status of the disk (0 = free, 1 = busy)
	beq $zero, $t0, $imm1, $imm2, 1, Free			# Disk is busy - Jumping to Free
	beq $zero, $zero, $zero, $ra, 0, 0				# Returning to the caller
	
Loop1:
	jal $ra, $zero, $zero, $imm2, 0, Free			# Jumping to Free
	jal $ra, $zero, $zero, $imm2, 0, Read			# Jumping to Read
	jal $ra, $zero, $zero, $imm2, 0, Free			# Jumping to Free
	jal $ra, $zero, $zero, $imm2, 0, Write			# Jumping to Write
	sub $s1, $s1, $imm1, $zero, 1, 0				# $s1 = $s1 - 1
	sub $s2, $s2, $imm1, $zero, 1, 0				# $s2 = $s2 - 1
	blt $zero, $s1, $zero, $imm2, 0, Exit			# If $s1 < 0, aka read all sectors, jump to Exit
	beq $zero, $zero, $zero, $imm2, 0, Loop1		# Otherwise, jump to Loop1
	
Read:
	out $zero, $imm1, $zero, $s1, 15, 0				# disksector = $s1 = sector number to read from
	out $zero, $imm1, $zero, $s0, 16, 0				# diskbuffer = $s0 = address in the memory
	out $zero, $imm1, $zero, $imm2, 14, 1			# diskcmd = 1 = read sector
		
	beq $zero, $zero, $zero, $ra, 0, 0				# Returning to the caller


Write:
	out $zero, $imm1, $zero, $s2, 15, 0				# disksector = $s2 = sector number to write to
	out $zero, $imm1, $zero, $s0, 16, 0				# diskbuffer = $s0 = address in the memory
	out $zero, $imm1, $zero, $imm2, 14, 2			# diskcmd = 2 = write sector
		
	beq $zero, $zero, $zero, $ra, 0, 0				# Returning to the caller


Exit:
	# Managing the stack
	lw $s0, $sp, $imm1, $zero, 0, 0					# Restoring $s0 from stack
	lw $s1, $sp, $imm1, $zero, 1, 0					# Restoring $s1 from stack
	lw $s2, $sp, $imm1, $zero, 2, 0					# Restoring $s2 from stack
	lw $ra, $sp, $imm1, $zero, 3, 0					# Restoring $ra from stack
	add $sp, $sp, $imm1, $zero, 4, 0				# Freeing space in the stack
	
	halt $zero, $zero, $zero, $zero, 0, 0			# Halt execution, exit simulator
	

