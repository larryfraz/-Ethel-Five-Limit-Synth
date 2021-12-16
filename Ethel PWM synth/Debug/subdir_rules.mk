################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"F:/ccs_12_2019_reinstall/ccsv8/tools/compiler/ti-cgt-msp430_18.1.4.LTS/bin/cl430" -vmsp --abi=coffabi -g --include_path="F:/ccs_12_2019_reinstall/ccsv8/ccs_base/msp430/include" --include_path="F:/ccs_12_2019_reinstall/ccsv8/tools/compiler/ti-cgt-msp430_18.1.4.LTS/include" --define=__MSP430G2452__ --diag_warning=225 --display_error_number --printf_support=minimal --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


