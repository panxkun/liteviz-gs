#include <liteviz/viewer.h>
#include <liteviz/dataloader.h>

int main(int argc, char** argv) {

    const char* usage = "Usage: ./liteviz [path_to_ply_file]\n";

    if (argc < 2) {
        std::cerr << usage;
        return 1;
    }

    const char* ply_file = argv[1];
    if (!std::filesystem::exists(ply_file)) {
        std::cerr << "File does not exist: " << ply_file << std::endl;
        return 1;
    }

    std::cout << "Loading Gaussian data from: " << ply_file << std::endl;

    GaussianData data = GaussianData::load_ply(ply_file);

    std::shared_ptr<LiteViewer> viewer = std::make_shared<LiteViewer>("LiteViz-GS", 1280, 720);

    viewer->draw(data);
}
