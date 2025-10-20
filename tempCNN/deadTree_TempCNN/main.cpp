#include "model.h"

using namespace std;

int main() {
    std::string save_path("/home/jo/Documents/S2/deadTree-obAgr/model.pt");
    // imput dim
    //num classes
    // sequ length
    // kernel size

    TempCNN model(13, 2, 45, 7, 128, 0.18203942949809093);
    torch::Tensor input = torch::randn({1, 13, 45});
    auto output = model->forward(input);
    std::cout << output << std::endl;
        torch::save(model, save_path);
   // model->save("model.pt");
    TempCNN loaded_model;
    //loaded_model->load("model.pt");
    return 0;
}
