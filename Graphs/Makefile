OBJDIR = build_files
BINDIR = binaries
CXX_GCC = g++
CXX_CLANG = clang++
CXX = $(CXX_GCC)
XML2_CXX_FLAGS = `xml2-config --cflags`
XML2_LD_FLAGS = `xml2-config --libs`
CXXFLAGS_OPT = -std=c++11 $(XML2_CXX_FLAGS) -O3 -flto -Wall -Werror
CXXFLAGS_PROFILE_VALGRIND = -std=c++11 $(XML2_CXX_FLAGS) -flto -Wall -Werror -g
XXFLAGS_PROFILE_GPROF = -std=c++11 $(XML2_CXX_FLAGS) -flto -Wall -Werror -pg
CXXFLAGS_DEBUG = -g -std=c++11 $(XML2_CXX_FLAGS) -Wall -Werror
CXXFLAGS = $(CXXFLAGS_OPT) -D_FILE_OFFSET_BITS=64
LDFLAGS = -lrt $(XML2_LD_FLAGS) -flto
LPSOLVE_LDFLAGS = -ldl lp_solve/liblpsolve55.a $(LDFLAGS)

CHACO_BASE_O = $(addprefix $(OBJDIR)/,chaco_parser.o)
CVCD_BASE_O = $(addprefix $(OBJDIR)/,structural_netlist_lexer.o vcd_lexer.o vcd_parser.o)
CENT_BASE_O = $(addprefix $(OBJDIR)/,structural_netlist_lexer.o vcd_lexer.o)
ETT_BASE_O = $(addprefix $(OBJDIR)/,structural_netlist_lexer.o)
GRAPH_BASE_O = $(addprefix $(OBJDIR)/,edge.o id_manager.o node.o port.o weight_score.o)
KLFM_BASE_O = $(addprefix $(OBJDIR)/,edge_klfm.o gain_bucket_manager_single_resource.o gain_bucket_manager_multi_resource_exclusive.o gain_bucket_manager_multi_resource_mixed.o gain_bucket_standard.o partition_engine_klfm.o partitioner_config.o preprocessor.o testbench_generator.o) \
              $(GRAPH_BASE_O)
LPSI_BASE_O = $(addprefix $(OBJDIR)/,lp_solve_interface.o) \
              $(CHACO_BASE_O) \
              $(GRAPH_BASE_O) \
              $(NTL_BASE_O)
NTL_BASE_O = $(addprefix $(OBJDIR)/,ntl_parser.o) \
             $(GRAPH_BASE_O)
SNP_BASE_O = $(addprefix $(OBJDIR)/,functional_edge.o structural_netlist_lexer.o structural_netlist_parser.o xilinx_functional_nodes.o)
VP_BASE_O = $(addprefix $(OBJDIR)/,structural_netlist_lexer.o vcd_lexer.o vcd_parser.o)
LPSI_BIN_O = $(OBJDIR)/lp_solve_interface_main.o \
             $(LPSI_BASE_O)
PM_BASE_O = $(addprefix $(OBJDIR)/,chaco_parser.o ntl_parser.o partitioner_config.o preprocessor.o xml_config_reader.o) \
            $(CHACO_BASE_O) \
            $(GRAPH_BASE_O) \
            $(KLFM_BASE_O) \
            $(NTL_BASE_O)

CVCD_BIN_O = $(OBJDIR)/compare_vcd_main.o \
             $(CVCD_BASE_O)
CENT_BIN_O = $(OBJDIR)/compare_entropy_main.o \
             $(CENT_BASE_O)
ETT_BIN_O = $(OBJDIR)/entropy_time_tracker_main.o \
            $(ETT_BASE_O)      
FNP_BIN_O = $(OBJDIR)/functional_netlist_parser_main.o \
            $(SNP_BASE_O)
FNPD_BIN_O = $(OBJDIR)/functional_netlist_parser_debug_main.o \
            $(SNP_BASE_O)
NFC_BIN_O = $(OBJDIR)/ntl_format_converter.o
PM_BIN_O = $(PM_BASE_O) $(OBJDIR)/partition_main.o
S2C_BIN_O = $(OBJDIR)/shan_to_csv_main.o \
            $(CENT_BASE_O)
SNP_BIN_O = $(OBJDIR)/structural_netlist_parser_main.o \
            $(SNP_BASE_O)
#VDG_BIN_O = $(OBJDIR)/vcd_dumpvars_generator_main.o \
            $(VP_BASE_O)
VP_BIN_O = $(VP_BASE_O) $(OBJDIR)/vcd_parser_main.o
WG_BIN_O = $(OBJDIR)/chaco_weight_generator.o \
           $(CHACO_BASE_O) \
           $(GRAPH_BASE_O)

BINARIES = $(addprefix $(BINDIR)/,partition_main compare_entropy compare_vcd entropy_time_tracker functional_netlist_parser functional_netlist_parser_debug lp_solve_interface ntl_format_converter shan_to_csv structural_netlist_parser vcd_parser weight_generator)

# ------------------------------------------------------------
# PROGRAMS

all: $(BINARIES) | $(BINDIR) $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

$(BINDIR):
	mkdir $(BINDIR)

$(BINDIR)/partition_main: $(PM_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/compare_entropy: $(CENT_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/compare_vcd: $(CVCD_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/entropy_time_tracker: $(ETT_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/functional_netlist_parser: $(FNP_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/functional_netlist_parser_debug: $(FNPD_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/lp_solve_interface: $(LPSI_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LPSOLVE_LDFLAGS)
	
$(BINDIR)/ntl_format_converter: $(NFC_BIN_O)	
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/shan_to_csv: $(S2C_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/structural_netlist_parser: $(SNP_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/vcd_dumpvars_generator: $(VDG_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/vcd_parser: $(VP_BIN_O)
	$(CXX) $(CXXFLAGS) $^ /localhome/gregerso/git/signalcontent/build_files/huffman.o -o $@ $(LDFLAGS)

$(BINDIR)/weight_generator: $(WG_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *o $(OBJDIR)/*o

# ------------------------------------------------------------
# HEADER DEPENDENCY DEFINITIONS

chaco_weight_generator_H = chaco_weight_generator.h
connection_descriptor_H = connection_descriptor.h
edge_H = edge.h
file_helpers_H = file_helpers.h
functional_edge_H = functional_edge.h
id_manager_H = id_manager.h
parser_interface_H = parser_interface.h
port_H = port.h
mps_name_hash_H = mps_name_hash.h
ntl_parser_H = ntl_parser.h
signal_entropy_info_H = signal_entropy_info.h
structural_netlist_lexer_H = structural_netlist_lexer.h
testbench_generator_H = testbench_generator.h
universal_macros_H = universal_macros.h
weight_score_H = weight_score.h

edge_klfm_H = $(edge_H) edge_klfm.h
functional_node_H = $(connection_descriptor_H) functional_node.h
lp_solve_interface_H = $(univeral_macros_H) lp_solve_interface.h
node_H = $(edge_H) $(port_H) node.h
vcd_lexer_H = $(file_helpers_H) vcd_lexer.h

chaco_parser_H = $(edge_H) $(node_H) $(parser_interface_H) chaco_parser.h
gain_bucket_entry_H = $(node_H) $(universal_macros_H) gain_bucket_entry.h
partitioner_config_H = $(node_H) partitioner_config.h
partition_engine_H = $(edge_klfm_H) partition_engine.h
vcd_parser_H = $(file_helpers_H) $(signal_entropy_H) $(vcd_lexer_H) vcd_parser.h
xilinx_functional_nodes_H = $(functional_node_H) $(structural_netlist_lexer_H) xilinx_functional_nodes.h

functional_node_factory_H = $(functional_node_H) $(xilinx_functional_nodes_H)
gain_bucket_interface_H = $(edge_klfm_H) $(gain_bucket_entry_H) gain_bucket_interface.h
preprocessor_H = $(node_H) $(partitioner_config_H) preprocessor.h
xml_config_reader_H = $(partitioner_config_H) xml_config_reader.h

gain_bucket_manager_H = $(node_H) $(gain_bucket_interface_H) $(gain_bucket_entry_H) $(partitioner_config_H) gain_bucket_manager.h
gain_bucket_standard_H = $(gain_bucket_entry_H) $(gain_bucket_interface_H) gain_bucket_standard.h
structural_netlist_parser_H = $(functional_node_factory_H) $(structural_netlist_lexer_H) structural_netlist_parser.h

gain_bucket_manager_single_resource_H = $(gain_bucket_entry_H) $(gain_bucket_manager_H) $(gain_bucket_standard_H) gain_bucket_manager_single_resource.h
gain_bucket_manager_multi_resource_exclusive_H = $(gain_bucket_entry_H) $(gain_bucket_manager_H) $(gain_bucket_standard_H) $(partitioner_config_H) gain_bucket_manager_multi_resource_exclusive.h
gain_bucket_manager_multi_resource_mixed_H = $(gain_bucket_entry_H) $(gain_bucket_manager_H) $(gain_bucket_standard_H) $(partitioner_config_H) gain_bucket_manager_multi_resource_mixed.h
partition_engine_klfm_H = $(edge_klfm_H) $(gain_bucket_entry_H) $(gain_bucket_manager_H) $(partition_engine_H) $(partitioner_config_H) partition_engine_klfm.h

# ------------------------------------------------------------
# COMPILER OBJECTS

$(OBJDIR)/partition_main.o: $(chaco_parser_H) $(id_manager_H) $(ntl_parser_H) $(partition_engine_H) $(partition_engine_klfm_H) $(preprocessor_H) $(testbench_generator_H) $(xml_config_reader_H) partition_main.cpp
	$(CXX) -c partition_main.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/chaco_parser.o: $(id_manager_H) $(chaco_parser_H) chaco_parser.cpp
	$(CXX) -c chaco_parser.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/chaco_weight_generator.o: $(chaco_parser_H) $(node_H) $(univeral_macros_H) $(chaco_weight_generator_H) chaco_weight_generator.cpp
	$(CXX) -c chaco_weight_generator.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/compare_entropy_main.o: $(file_helpers_H) $(structural_netlist_lexer_H) compare_entropy_main.cpp
	$(CXX) -c compare_entropy_main.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/compare_vcd_main.o: $(file_helpers_H) $(signal_entropy_info_H) $(vcd_parser_H) compare_vcd_main.cpp
	$(CXX) -c compare_vcd_main.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/edge.o: $(edge_H) edge.cpp
	$(CXX) -c edge.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/edge_klfm.o: $(edge_klfm_H) edge_klfm.cpp
	$(CXX) -c edge_klfm.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/entropy_time_tracker_main.o: $(structural_netlist_lexer_H) entropy_time_tracker_main.cpp
	$(CXX) -c entropy_time_tracker_main.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/functional_edge.o: $(functional_node_H) $(functional_edge_H) functional_edge.cpp
	$(CXX) -c functional_edge.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/functional_netlist_parser_main.o: $(functional_edge_H) $(functional_node_H) $(structural_netlist_parser_H) functional_netlist_parser_main.cpp
	$(CXX) -c $(CXXFLAGS) functional_netlist_parser_main.cpp -o $(OBJDIR)/functional_netlist_parser_main.o

$(OBJDIR)/functional_netlist_parser_debug_main.o: $(functional_edge_H) $(functional_node_H) $(structural_netlist_parser_H) functional_netlist_parser_debug_main.cpp
	$(CXX) -c $(CXXFLAGS) functional_netlist_parser_debug_main.cpp -o $(OBJDIR)/functional_netlist_parser_debug_main.o

$(OBJDIR)/gain_bucket_manager_single_resource.o: $(gain_bucket_manager_single_resource_H) gain_bucket_manager_single_resource.cpp
	$(CXX) -c gain_bucket_manager_single_resource.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/gain_bucket_manager_multi_resource_exclusive.o: $(universal_macros_H) $(gain_bucket_manager_multi_resource_exclusive_H) gain_bucket_manager_multi_resource_exclusive.cpp
	$(CXX) -c gain_bucket_manager_multi_resource_exclusive.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/gain_bucket_manager_multi_resource_mixed.o: $(universal_macros_H) $(weight_score_H) $(gain_bucket_manager_multi_resource_mixed_H) gain_bucket_manager_multi_resource_mixed.cpp
	$(CXX) -c gain_bucket_manager_multi_resource_mixed.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/gain_bucket_standard.o: $(gain_bucket_standard_H) gain_bucket_standard.cpp
	$(CXX) -c gain_bucket_standard.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/id_manager.o: $(id_manager_H) id_manager.cpp
	$(CXX) -c id_manager.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/lp_solve_interface.o: $(chaco_parser_H) $(edge_H) $(mps_name_hash_H) $(node_H) $(ntl_parser_H) $(lp_solve_interface_H) lp_solve_interface.cpp
	$(CXX) -c lp_solve_interface.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/lp_solve_interface_main.o: $(lp_solve_interface_H) lp_solve_interface_main.cpp
	$(CXX) -c lp_solve_interface_main.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/node.o: $(id_manager_H) $(universal_macros_H) $(node_H) $(weight_score_H) node.cpp
	$(CXX) -c node.cpp $(CXXFLAGS) -o $@
	
$(OBJDIR)/ntl_format_converter.o: ntl_format_converter.cpp
	$(CXX) -c ntl_format_converter.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/partition_engine_klfm.o: $(gain_bucket_manager_single_resource_H) $(gain_bucket_manager_multi_resource_exclusive_H) $(gain_bucket_manager_multi_resource_mixed_H) $(id_manager_H) $(mps_name_hash_H) $(universal_macros_H) $(weight_score_H) $(partition_engine_klfm_H) partition_engine_klfm.cpp
	$(CXX) -c partition_engine_klfm.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/partitioner_config.o: $(universal_macros_H) $(partitioner_config_H) partitioner_config.cpp
	$(CXX) -c partitioner_config.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/port.o: $(id_manager_H) $(port_H) port.cpp
	$(CXX) -c port.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/preprocessor.o: $(universal_macros_H) $(preprocessor_H) preprocessor.cpp
	$(CXX) -c preprocessor.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/ntl_parser.o: $(edge_H) $(id_manager_H) $(node_H) $(universal_macros_H) $(ntl_parser_H) ntl_parser.cpp
	$(CXX) -c ntl_parser.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/shan_to_csv_main.o: $(file_helpers_H) $(structural_netlist_lexer_H) shan_to_csv_main.cpp
	$(CXX) -c shan_to_csv_main.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/structural_netlist_lexer.o: $(structural_netlist_lexer_H) structural_netlist_lexer.cpp
	$(CXX) -c structural_netlist_lexer.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/structural_netlist_parser.o: $(functional_edge_H) $(functional_node_H) $(functional_node_factory_H) $(structural_netlist_lexer_H) $(universal_macros_H) $(structural_netlist_parser_H) structural_netlist_parser.cpp
	$(CXX) -c structural_netlist_parser.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/structural_netlist_parser_main.o: $(functional_edge_H) $(functional_node_H) $(structural_netlist_parser_H) structural_netlist_parser_main.cpp
	$(CXX) -c $(CXXFLAGS) structural_netlist_parser_main.cpp -o $(OBJDIR)/structural_netlist_parser_main.o

$(OBJDIR)/testbench_generator.o: $(testbench_generator_H) testbench_generator.cpp
	$(CXX) -c testbench_generator.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/vcd_dumpvars_generator_main.o: $(vcd_parser_H) vcd_dumpvars_generator_main.cpp
	$(CXX) -c vcd_dumpvars_generator_main.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/vcd_lexer.o: $(universal_macros_H) $(vcd_lexer_H) vcd_lexer.cpp
	$(CXX) -c vcd_lexer.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/vcd_parser.o: $(universal_macros_H) vcd_parser.cpp
	$(CXX) -c vcd_parser.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/vcd_parser_main.o: $(signal_entropy_info_H) $(vcd_parser_H) vcd_parser_main.cpp
	$(CXX) -c vcd_parser_main.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/weight_score.o: $(weight_score_H) weight_score.cpp
	$(CXX) -c weight_score.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/xilinx_functional_nodes.o: $(functional_edge_H) $(functional_node_H) xilinx_functional_nodes.cpp
	$(CXX) -c xilinx_functional_nodes.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/xml_config_reader.o: $(universal_macros_H) $(xml_config_reader_H) xml_config_reader.cpp
	$(CXX) -c xml_config_reader.cpp $(CXXFLAGS) -o $@
