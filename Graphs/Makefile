OBJDIR = build_files
BINDIR = binaries
#COMPILER = /app/usr/local/bin/g++
#COMPILER = g++
CXX = clang++
XML2_FLAGS = `xml2-config --cflags`
CXXFLAGS = -g -std=c++11 $(XML2_FLAGS)
#CFLAGS = $(XML2_FLAGS) -flto -O3 -std=gnu++0x -c
#LDFLAGS = -flto -lrt -v
LDFLAGS = -lrt -v `xml2-config --libs`
LPSOLVE_LDFLAGS = lp_solve/liblpsolve55.a $(LDFLAGS)

PMBASE_OBJS = $(addprefix $(OBJDIR)/,chaco_parser.o edge.o edge_klfm.o gain_bucket_manager_single_resource.o gain_bucket_manager_multi_resource_exclusive.o gain_bucket_manager_multi_resource_mixed.o gain_bucket_standard.o id_manager.o node.o partition_engine_klfm.o partitioner_config.o port.o preprocessor.o processed_netlist_parser.o testbench_generator.o weight_score.o xml_config_reader.o)
PM_OBJS = $(PMBASE_OBJS) $(OBJDIR)/partition_main.o

SNPBASE_OBJS = $(OBJDIR)/structural_netlist_parser.o
SNP_OBJS = $(SNPBASE_OBJS) $(OBJDIR)/structural_netlist_parser_main.o

VDG_OBJS = $(addprefix $(OBJDIR)/,vcd_parser.o vcd_dumpvars_generator_main.o)

VPBASE_OBJS = $(addprefix $(OBJDIR)/,vcd_parser.o)
VP_OBJS = $(VPBASE_OBJS) $(OBJDIR)/vcd_parser_main.o

WG_OBJS = $(OBJDIR)/chaco_weight_generator.o

OBJS = $(PM_OBJS) $(SNP_OBJS) $(VDG_OBJS) $(VP_OBJS) $(WG_OBJS)

BINARIES = $(addprefix $(BINDIR)/,partition_main structural_netlist_parser vcd_dumpvars_generator vcd_parser weight_generator)

# ------------------------------------------------------------
# PROGRAMS

all: $(BINARIES) | $(BINDIR) $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

$(BINDIR):
	mkdir $(BINDIR)

$(BINDIR)/partition_main: $(PM_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/structural_netlist_parser: $(SNP_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/vcd_dumpvars_generator: $(VDG_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/vcd_parser: $(VP_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BINDIR)/weight_generator: $(WG_OBJS) $(PMBASE_OBJS)
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
processed_netlist_parser_H = processed_netlist_parser.h
structural_netlist_parser_H = structural_netlist_parser.h
testbench_generator_H = testbench_generator.h
universal_macros_H = universal_macros.h
vcd_parser_H = vcd_parser.h
weight_score_H = weight_score.h

edge_klfm_H = $(edge_H) edge_klfm.h
lp_solve_interface_H = $(univeral_macros_H)
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

$(OBJDIR)/partition_main.o: $(chaco_parser_H) $(id_manager_H) $(partition_engine_H) $(partition_engine_klfm_H) $(preprocessor_H) $(processed_netlist_parser_H) $(testbench_generator_H) $(xml_config_reader_H) partition_main.cpp
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

$(OBJDIR)/lp_solve_interface.o: $(chaco_parser_H) $(edge_H) $(node_H) $(processed_netlist_parser_H) $(lp_solve_interface_H) lp_solve_interface.cpp
	$(CXX) -c lp_solve_interface.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/node.o: $(id_manager_H) $(universal_macros_H) $(node_H) $(weight_score_H) node.cpp
	$(CXX) -c node.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/partition_engine_klfm.o: $(gain_bucket_manager_single_resource_H) $(gain_bucket_manager_multi_resource_exclusive_H) $(gain_bucket_manager_multi_resource_mixed_H) $(id_manager_H) $(universal_macros_H) $(weight_score_H) $(partition_engine_klfm_H) partition_engine_klfm.cpp
	$(CXX) -c partition_engine_klfm.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/partitioner_config.o: $(universal_macros_H) $(partitioner_config_H) partitioner_config.cpp
	$(CXX) -c partitioner_config.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/port.o: $(id_manager_H) $(port_H) port.cpp
	$(CXX) -c port.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/preprocessor.o: $(universal_macros_H) $(preprocessor_H) preprocessor.cpp
	$(CXX) -c preprocessor.cpp $(CXXFLAGS) -o $@

$(OBJDIR)/processed_netlist_parser.o: $(edge_H) $(id_manager_H) $(node_H) $(processed_netlist_parser_H) $(universal_macros_H) processed_netlist_parser.cpp
	$(CXX) -c processed_netlist_parser.cpp $(CXXFLAGS) -o $@

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
