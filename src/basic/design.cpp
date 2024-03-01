#include "../basic/design.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include "../basic/netlist.h"

using namespace std;

Design::~Design() {
}

// #include <sys/mman.h>  // For mmap()
// #include <sys/stat.h>  // For stat()
// #include <fcntl.h>     // For open()
// #include <unistd.h>    // For close()
// #include <cstring>     // For strerror()
// #include <iostream>
// #include <vector>
// #include <string>

// bool Design::readCap(const std::string& filename) {
//     // Open file
//     int fd = open(filename.c_str(), O_RDONLY);
//     if (fd == -1) {
//         std::cerr << "Error opening file: " << strerror(errno) << std::endl;
//         return false;
//     }

//     // Get file size
//     struct stat fileInfo;
//     if (fstat(fd, &fileInfo) == -1) {
//         std::cerr << "Error getting the file size: " << strerror(errno) << std::endl;
//         close(fd);
//         return false;
//     }

//     // Memory-map the file
//     char* mapped = static_cast<char*>(mmap(nullptr, fileInfo.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
//     if (mapped == MAP_FAILED) {
//         std::cerr << "Error mapping the file: " << strerror(errno) << std::endl;
//         close(fd);
//         return false;
//     }

//     // Initialize istringstream with the mapped file content
//     std::istringstream iss(std::string(mapped, fileInfo.st_size));

//     // Read and parse the file content
//     iss >> dimension.n_layers >> dimension.x_size >> dimension.y_size;
//     iss >> metrics.UnitLengthWireCost >> metrics.UnitViaCost;
    
//     metrics.OFWeight.clear();
//     double OFWeight;
//     for (int i = 0; i < dimension.n_layers; i++) {
//         iss >> OFWeight;
//         metrics.OFWeight.push_back(OFWeight);
//     }
    
//     dimension.hEdge.clear();
//     int hEdge;
//     for (int i = 0; i < dimension.x_size - 1; i++) {
//         iss >> hEdge;
//         dimension.hEdge.push_back(hEdge);
//     }
    
//     dimension.vEdge.clear();
//     int vEdge;
//     for (int i = 0; i < dimension.y_size - 1; i++) {
//         iss >> vEdge;
//         dimension.vEdge.push_back(vEdge);
//     }
    
//     layers.clear();
//     char name[1000];
//     for (int i = 0; i < dimension.n_layers; i++) {
//         Layer layer;
//         iss >> name >> layer.direction >> layer.minLength;
//         layer.id = i;
//         layer.capacity.resize(dimension.y_size, std::vector<double>(dimension.x_size, 0.0));
//         for (int y = 0; y < dimension.y_size; y++) {
//             for (int x = 0; x < dimension.x_size; x++) {
//                 iss >> layer.capacity[y][x];
//             }
//         }
//         layers.push_back(layer);
//     }

//     // Unmap memory and close the file
//     munmap(mapped, fileInfo.st_size);
//     close(fd);

//     return true;
// }

bool Design::readCap(const string& filename) {
    FILE* inputFile = fopen(filename.c_str(), "r");
    char line[1000];
    if (inputFile == NULL) {
        printf("Error opening file.\n");
        return false;
    }
    fscanf(inputFile, "%d %d %d", &dimension.n_layers, &dimension.x_size, &dimension.y_size);
    // fgets(line, sizeof(line), inputFile)
    fscanf(inputFile, "%lf %lf", &metrics.UnitLengthWireCost, &metrics.UnitViaCost);
    parameters.UnitViaCost = metrics.UnitViaCost;
    cout << "UnitViaCost: " << parameters.UnitViaCost << endl;
    double OFWeight;
    metrics.OFWeight.reserve(dimension.n_layers);
    for (int i = 0; i < dimension.n_layers; i++) {
        fscanf(inputFile, "%lf", &OFWeight);
        metrics.OFWeight.emplace_back(OFWeight);
    }
    dimension.hEdge.reserve(dimension.x_size);
    int hEdge;
    for (int i = 0; i < dimension.x_size-1; i++) {
        fscanf(inputFile, "%d", &hEdge);
        dimension.hEdge.emplace_back(hEdge);
    }
    dimension.vEdge.reserve(dimension.y_size);
    int vEdge;
    for (int i = 0; i < dimension.y_size-1; i++) {
        fscanf(inputFile, "%d", &vEdge);
        dimension.vEdge.emplace_back(vEdge);
    }
    char name[1000];
    for (int i = 0; i < dimension.n_layers; i++) {
        Layer layer;
        layer.id = i;
        fscanf(inputFile, "%s %d %lf", name, &layer.direction, &layer.minLength);
        vector<vector<double>> cap(dimension.y_size, vector<double>(dimension.x_size, 0));
        for (int i = 0; i < dimension.y_size; i++) {
            for (int j = 0; j < dimension.x_size; j++) {
                fscanf(inputFile, "%lf", &cap[i][j]);
            }
        }
        layer.capacity = cap;
        layers.push_back(layer);
    }
    fclose(inputFile);
    return true;
}

void replaceChars(char *str) {
    while (*str != '\0') {
        if (*str == '[' || *str == ']' || *str == '(' || *str == ')' || *str == ',') {
            *str = ' ';
        }
        str++;
    }
}

bool Design::readNet(const string& filename) {

    vector<Net> nets;
    vector<Pin> pins;
    vector<Point> points;

    char net_name[1000];

    int net_id = 0;
    int pin_id = 0;
    int point_id = 0;

    FILE* inputFile = fopen(filename.c_str(), "r");
    char line[1000];
    if (inputFile == NULL) {
        printf("Error opening file.\n");
        return false;
    }
    while (fgets(line, sizeof(line), inputFile)){
        size_t ln = strlen(line) - 1;
        if (line[ln] == '\n') line[ln] = '\0';
        string str(line);
        Net net(net_id++, str);
        vector<int> pin_ids;
        fgets(line, sizeof(line), inputFile); // consumes the "("
        while (fgets(line, sizeof(line), inputFile)) {
            int layer, x, y;
            if (strcmp(line, ")\n") == 0){
                break; // end of net
            }
            Pin pin(pin_id++, net_id);
            vector<int> point_ids;
            replaceChars(line);
            char *token = strtok(line, " \t\n");
            while (token != NULL) {
                layer = atoi(token);
                token = strtok(NULL, " \t\n");
                if (token != NULL) {
                    x = atoi(token);
                    token = strtok(NULL, " \t\n");
                } else {
                    break; 
                }
                if (token != NULL) {
                    y = atoi(token);
                    token = strtok(NULL, " \t\n");
                } else {
                    break;
                }
                Point point(point_id++, net_id, layer, x, y);
                point_ids.push_back(point.id);
                points.emplace_back(point);
            }
            pin.point_ids = point_ids;
            pin_ids.push_back(pin.id);
            pins.emplace_back(pin);
        }
        net.pin_ids = pin_ids;
        nets.emplace_back(net);
    }

    netlist.n_nets = nets.size();
    netlist.n_pins = pins.size();
    netlist.n_points = points.size();
    netlist.nets = nets;
    netlist.pins = pins;
    netlist.points = points;
    fclose(inputFile);
    return true;
}
