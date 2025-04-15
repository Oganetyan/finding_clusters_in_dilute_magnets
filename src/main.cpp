#include <iostream>
#include <fstream>
#include <argparse/argparse.hpp>
#include "include/config.h"
#include "include/lattice.h"

int main(int argc, char **argv)
{
    argparse::ArgumentParser program("finding_clusters_in_dilute_magnets");
    program.add_argument("-c", "--config")
        .default_value(std::string("../data/configs/default.json"));
    program.add_argument("-l", "--lattice")
        .default_value(std::string("ALL"))
        .choices("SC", "BCC", "FCC", "ALL");

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err)
    {
        std::cerr << err.what() << "\n";
        std::cerr << program;
        return 1;
    }

    std::string config_path = program.get<std::string>("--config");
    std::string lattice_type = program.get<std::string>("--lattice");

    try
    {
        CFG.load(config_path);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    std::vector<long double> average_cluster_counts;
    std::vector<long double> average_cluster_size;

    auto simulate_lattice = [&](const std::string &lattice_name)
    {
        std::cout << "Lattice type is " << lattice_name << std::endl;

        Lattice lattice(lattice_name);
        const double initial_concentration = CFG.get<double>("simulation.initial_concentration");
        const double final_concentration = CFG.get<double>("simulation.final_concentration");
        const double concentration_step = CFG.get<double>("simulation.concentration_step");
        const int num_configuration = CFG.get<int>("simulation.num_configurations");
        const int lattice_volume = CFG.lattice_volume();
        const int progres_step = num_configuration / 100;

        for (double concentration = initial_concentration; concentration <= final_concentration + 1e-3; concentration += concentration_step)
        {
            int non_magnetic_count = std::ceil((1 - concentration) * lattice_volume);
            long total_clusters = 0;
            long total_clusters_size = 0;

            for (int configuration = 0; configuration < num_configuration; ++configuration)
            {
                if (configuration % progres_step == 0 || configuration == num_configuration - 1)
                {
                    int persent = (configuration * 100) / (num_configuration - 1);
                    std::cout << "\rConcentration: " << concentration
                              << " | Completed: " << persent << "%   " << std::flush;
                }
                lattice.initialize();
                lattice.replace_random_spins(non_magnetic_count);
                auto clusters = lattice.find_clusters();

                total_clusters += clusters.size();
                for (const auto &cluster : clusters)
                    total_clusters_size += cluster.size();
            }

            average_cluster_counts.push_back(static_cast<long double>(total_clusters) / num_configuration);
            average_cluster_size.push_back(static_cast<long double>(total_clusters_size) / (total_clusters ? total_clusters : 1));
        }
        std::cout << std::endl;
        std::ofstream output_file("../data/output_txt/clusters_" + lattice_name + ".txt");
        if (output_file.is_open())
        {
            double concentration = initial_concentration;
            for (int i = 0; i < average_cluster_counts.size(); ++i)
            {
                output_file << concentration << "\t"
                            << average_cluster_counts[i] << "\t"
                            << average_cluster_size[i] << "\n";
                concentration += concentration_step;
            }
            output_file.close();
            std::cout << "Data saved!" << std::endl;
        }
        else
            std::cerr << "Failed to open file" << std::endl;
        std::cout << std::endl;

        average_cluster_counts.clear();
        average_cluster_size.clear();
    };

    if (lattice_type == "SC" || lattice_type == "ALL")
        simulate_lattice("SC");
    if (lattice_type == "BCC" || lattice_type == "ALL")
        simulate_lattice("BCC");
    if (lattice_type == "FCC" || lattice_type == "ALL")
        simulate_lattice("FCC");

    return 0;
}