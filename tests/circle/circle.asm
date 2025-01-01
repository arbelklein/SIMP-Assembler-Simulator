#circle.asm

Main:
	# Manging the stack
	sll $sp, $imm1, $imm2, $zero, 1, 11				# Setting $sp = 1 << 11 = 2048
	sub $sp, $sp, $imm1, $zero, 4, 0				# Allocating space in the stack for 2 items
	sw $zero, $sp, $imm1, $s0, 0, 0					# Storing $s0 in the stack
	sw $zero, $sp, $imm1, $s1, 1, 0					# Storing $s1 in the stack
	sw $zero, $sp, $imm1, $s2, 2, 0					# Storing $s2 in the stack
	sw $zero, $sp, $imm1, $ra, 3, 0					# Storing $ra in the stack
	
	lw $s0, $imm1, $zero, $zero, 256, 0				# Loading the radius from 0x100 (256) address to $s0
	add $s1, $zero, $zero, $zero, 0, 0				# Setting $s1 = 0, aka y = 0, aka the rows

LoopY:	
	add $s2, $zero, $zero, $zero, 0, 0				# Setting $s2 = 0, aka x = 0, aka the columns
	beq $zero, $zero, $zero, $imm2, 0, LoopX		# Jumping to LoopX

LoopX:
	# Calculating the distance from the center of the monitor
	sub $t0, $s2, $imm1, $zero, 128, 0				# Calculating $t0 = $s2 - 128 = x - 128
	mac $t0, $t0, $t0, $zero, 0, 0					# Calculating $t0 = $t0 * $t0 = (x - 128)^2
	sub $t1, $s1, $imm1, $zero, 128, 0				# Calculating $t0 = $s1 - 128 = y - 128
	mac $t1, $t1, $t1, $zero, 0, 0					# Calculating $t1 = $t1 * $t1 = (y - 128)^2
	add $t2, $t1, $t0, $zero, 0, 0					# Calculating $t2 = $t1 + $t0 = (y - 128)^2 + (x - 128)^2
	mac $t0, $s0, $s0, $zero, 0, 0					# Calculating $t0 = $s0 * $s0 = radius^2
	
	# Calculating the address of the pixel
	mac $t1, $s1, $imm1, $s2, 256, 0				# $t1 = $s1 * 256 + $s2 = y * 256 + x
	
	# Deciding which color to draw
	ble $zero, $t2, $t0, $imm2, 0, Color_White		# If $t2 <= $t0, aka if (y - 128)^2 + (x - 128)^2 <= radius^2, jump to Color_White
	out $zero, $imm1, $zero, $t1, 20, 0				# monitoraddr = $t1 = address of the pixel
	out $zero, $imm1, $zero, $imm2, 21, 0			# monitordata = 0 = black
	out $zero, $imm1, $zero, $imm2, 22, 1			# monitorcmd = 1 = write to monitor
	beq $zero, $zero, $zero, $imm2, 0, IncrementX	# Jumping to IncrementX

Color_White:
	out $zero, $imm1, $zero, $t1, 20, 0				# monitoraddr = $t1 = address of the pixel
	out $zero, $imm1, $zero, $imm2, 21, 255			# monitordata = 255 = white
	out $zero, $imm1, $zero, $imm2, 22, 1			# monitorcmd = 1 = write to monitor
	beq $zero, $zero, $zero, $imm2, 0, IncrementX	# Jumping to IncrementX

IncrementX:
	add $s2, $s2, $imm1, $zero, 1, 0				# Setting $s2 = $s2 + 1, aka x = x + 1
	blt $zero, $s2, $imm1, $imm2, 256, LoopX		# If $s2 < 256, aka if x < 256, jump to LoopX
	beq $zero, $zero, $zero, $imm2, 0, IncrementY	# Jumping to IncrementY

IncrementY:
	add $s1, $s1, $imm1, $zero, 1, 0				# Setting $s1 = $s1 + 1, aka y = y + 1
	blt $zero, $s1, $imm1, $imm2, 256, LoopY		# If $s1 < 256, aka if y < 256, jump to LoopY
	beq $zero, $zero, $zero, $imm2, 0, Exit			# Jumping to Exit

Exit:
	# Managing the stack
	lw $s0, $sp, $imm1, $zero, 0, 0					# Restoring $s0 from stack
	lw $s1, $sp, $imm1, $zero, 1, 0					# Restoring $s1 from stack
	lw $s2, $sp, $imm1, $zero, 2, 0					# Restoring $s2 from stack
	lw $ra, $sp, $imm1, $zero, 3, 0					# Restoring $ra from stack
	add $sp, $sp, $imm1, $zero, 4, 0				# Freeing space in the stack
	
	halt $zero, $zero, $zero, $zero, 0, 0			# Halt execution, exit simulator
	