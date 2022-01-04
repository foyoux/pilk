include_directories(../interface)
aux_source_directory(. SKP_Silk_SDK)
add_library(SKP_Silk_SDK ${SKP_Silk_SDK})