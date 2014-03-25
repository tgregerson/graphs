OBJDIR = build_files
BINDIR = binaries
CXX_GCC = g++
CXX_CLANG = clang++
CXX = $(CXX_CLANG)
XML2_FLAGS = `xml2-config --cflags`
CXXFLAGS_OPT = -std=c++11 $(XML2_FLAGS) -O3 -flto -Wall -Werror
CXXFLAGS_DEBUG = -g -std=c++11 $(XML2_FLAGS) -Wall -Werror
CXXFLAGS = $(CXXFLAGS_OPT)
LDFLAGS = -lrt -v `xml2-config --libs` -flto
LPSOLVE_LDFLAGS = -ldl lp_solve/liblpsolve55.a $(LDFLAGS)

CHACO_BASE_O = $(addprefix $(OBJDIR)/,chaco_parser.o)
GRAPH_BASE_O = $(addprefix $(OBJDIR)/,edge.o id_manager.o node.o port.o weight_score.o)
KLFM_BASE_O = $(addprefix $(OBJDIR)/,edge_klfm.o gain_bucket_manager_single_resource.o gain_bucket_manager_multi_resource_exclusive.o gain_bucket_manager_multi_resource_mixed.o gain_bucket_standard.o partition_engine_klfm.o partitioner_config.o preprocessor.o testbench_generator.o) \
              $(GRAPH_BASE_O)
LPSI_BASE_O = $(addprefix $(OBJDIR)/,lp_solve_interface.o) \
              $(CHACO_BASE_O) \
              $(GRAPH_BASE_O) \
              $(NTL_BASE_O)
NTL_BASE_O = $(addprefix $(OBJDIR)/,ntl_parser.o) \
             $(GRAPH_BASE_O)
SNP_BASE_O = $(addprefix $(OBJDIR)/,structural_netlist_parser.o)
VP_BASE_O = $(addprefix $(OBJDIR)/,vcd_parser.o)
LPSI_BIN_O = $(OBJDIR)/lp_solve_interface_main.o \
             $(LPSI_BASE_O)
PM_BASE_O = $(addprefix $(OBJDIR)/,chaco_parser.o ntl_parser.o partitioner_config.o preprocessor.o xml_config_reader.o) \
            $(CHACO_BASE_O) \
            $(GRAPH_BASE_O) \
            $(KLFM_BASE_O) \
            $(NTL_BASE_O)

NFC_BIN_O = $(OBJDIR)/ntl_format_converter.o
PM_BIN_O = $(PM_BASE_O) $(OBJDIR)/partition_main.o
SNP_BIN_O = $(OBJDIR)/structural_netlist_parser_main.o \
            $(SNP_BASE_O)
VDG_BIN_O = $(addprefix $(OBJDIR)/,vcd_parser.o vcd_dumpvars_generator_main.o)
VP_BIN_O = $(VP_BASE_O) $(OBJDIR)/vcd_parser_main.o
WG_BIN_O = $(OBJDIR)/chaco_weight_generator.o \
           $(CHACO_BASE_O) \
           $(GRAPH_BASE_O)

BINARIES = $(addprefix $(BINDIR)/,partition_main lp_solve_interface ntl_format_converter structural_netlist_parser vcd_dumpvars_generator vcd_parser weight_generator)

# ------------------------------------------------------------
# PROGRAMS

all: $(BINARIES) | $(BINDIR) $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

$(BINDIR):
	mkdir $(BINDIR)

$(BINDIR)/partition_main: $(PM_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/lp_solve_interface: $(LPSI_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LPSOLVE_LDFLAGS)
	
$(BINDIR)/ntl_format_converter: $(NFC_BIN_O)	
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/structural_netlist_parser: $(SNP_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/vcd_dumpvars_generator: $(VDG_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/vcd_parser: $(VP_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/weight_generator: $(WG_BIN_O)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *o $(OBJDIR)/*o

# ------------------------------------------------------------
# HEADER DEPENDENCY DEFINITIONS

chaco_weight_generator_H = chaco_weight_generator.h
edge_H = edge.h
id_manager_H = id_manager.h
parser_interface_H = parser_interface.h
port_H = port.h
mps_name_hash_H = mps_name_hash.h
ntl_parser_H = ntl_parser.h
structural_netlist_parser_H = structural_netlist_parser.h
testbench_generator_H = testbench_generator.h
universal_macros_H = universal_macros.h
vcd_parser_H = vcd_parser.h
weight_score_H = weight_score.h

edge_klfm_H = $(edge_H) edge_klfm.h
lp_solve_interface_H = $(univeral_macros_H) lp_solve_interface.h
node_H = $(edge_H) $(port_H) node.h

chaco_parser_H = $(edge_H) $(node_H) $(parser_interface_H) chaco_parser.h
gain_bucket_entry_H = $(node_H) $(universal_macros_H) gain_bucket_entry.h
partitioner_config_H = $(node_H) partitioner_config.h
partition_engine_H = $(edge_klfm_H) partition_engine.h

gain_bucket_interface_H = $(edge_klfm_H) $(gain_bucket_entry_H) gain_bucket_interface.h
preprocessor_H = $(node_H) $(partitioner_config_H) preprocessor.h
xml_config_reader_H = $(partitioner_config_H) xml_config_reader.h

gain_bucket_manager_H = $(node_H) $(gain_bucket_interface_H) $(gain_bucket_entry_H) $(partitioner_config_H) gain_bucket_manager.h
gain_bucket_standard_H = $(gain_bucket_entry_H) $(gain_bucket_interface_H) gain_bucket_standard.h

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

$(OBJDIR)/edge.o: $(edge_H) edge.cpp
	$(CXX) -c edge.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/edge_klfm.o: $(edge_klfm_H) edge_klfm.cpp
	$(CXX) -c edge_klfm.cpp $(CXXFLAGS) -o $@

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

$(OBJDIR)/structural_netlist_parser.o: $(universal_macros_H) $(structural_netlist_parser_H) structural_netlist_parser.cpp
	$(CXX) -c structural_netlist_parser.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/structural_netlist_parser_main.o: $(structural_netlist_parser_H) structural_netlist_parser_main.cpp
	$(CXX) -c $(CXXFLAGS) structural_netlist_parser_main.cpp -o $(OBJDIR)/structural_netlist_parser_main.o

$(OBJDIR)/testbench_generator.o: $(testbench_generator_H) testbench_generator.cpp
	$(CXX) -c testbench_generator.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/vcd_dumpvars_generator_main.o: $(vcd_parser_H) vcd_dumpvars_generator_main.cpp
	$(CXX) -c vcd_dumpvars_generator_main.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/vcd_parser.o: $(universal_macros_H) vcd_parser.cpp
	$(CXX) -c vcd_parser.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/vcd_parser_main.o: $(vcd_parser_H) vcd_parser_main.cpp
	$(CXX) -c vcd_parser_main.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/weight_score.o: $(weight_score_H) weight_score.cpp
	$(CXX) -c weight_score.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/xml_config_reader.o: $(universal_macros_H) $(xml_config_reader_H) xml_config_reader.cpp
	$(CXX) -c xml_config_reader.cpp $(CXXFLAGS) -o $@
