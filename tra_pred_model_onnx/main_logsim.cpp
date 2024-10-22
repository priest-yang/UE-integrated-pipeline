#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include <vector>
#include <thread>
#include "argparse.hpp"
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include "log_reader.hpp"

class ModelRunner {
private:
    Ort::Env env;
    Ort::SessionOptions session_options;
    Ort::Session session;
    std::vector<std::string> input_node_names;
    std::vector<std::string> output_node_names;
    int feature_dim;
    std::deque<std::vector<float>> buffer;
    size_t capacity;
    int cnt = 0;
    LogReader log_reader;
    int processed_lines = 0;
    std::chrono::duration<double> elapsed;
    
    

public:
    ModelRunner(const std::string& model_path, int feature_dim, size_t capacity, const std::string& log_dir)
        : feature_dim(feature_dim), capacity(capacity), log_reader(log_dir),
          env(ORT_LOGGING_LEVEL_WARNING, "ModelRunner"),
          session_options(), session(nullptr) {
        // Set session options if needed
        session_options.SetIntraOpNumThreads(1);

        // Load the model
        try {
            session = Ort::Session(env, model_path.c_str(), session_options);

            // Get input and output node names
            size_t num_input_nodes = session.GetInputCount();
            size_t num_output_nodes = session.GetOutputCount();
            Ort::AllocatorWithDefaultOptions allocator;

            // Get input node names
            for (size_t i = 0; i < num_input_nodes; i++) {
                Ort::AllocatedStringPtr input_name = session.GetInputNameAllocated(i, allocator);
                input_node_names.push_back(input_name.get());
            }

            // Get output node names
            for (size_t i = 0; i < num_output_nodes; i++) {
                Ort::AllocatedStringPtr output_name = session.GetOutputNameAllocated(i, allocator);
                output_node_names.push_back(output_name.get());
            }

            std::cout << "Model loaded successfully." << std::endl;
        } catch (const Ort::Exception& exception) {
            std::cerr << "Error loading the model: " << exception.what() << std::endl;
            exit(-1);
        }
    }

    void feedModel() {
        printf("Checking for new data\n");
        if (!log_reader.has_new_data()) return;
        printf("New data available\n");
        auto start = std::chrono::high_resolution_clock::now();
        
        size_t batch_size = 1;  // Add a batch dimension of 1

        buffer = log_reader._newest_data();

        size_t sequence_length = buffer.size();
        size_t input_tensor_size = batch_size * sequence_length * feature_dim;
        std::vector<float> input_tensor_values;
        input_tensor_values.reserve(input_tensor_size);

        for (const auto& vec : buffer) {
            input_tensor_values.insert(input_tensor_values.end(), vec.begin(), vec.end());
        }

        // Correct input shape must be [batch_size, sequence_length, feature_dim]
        std::vector<int64_t> input_shape = {static_cast<int64_t>(batch_size), static_cast<int64_t>(sequence_length), feature_dim};

        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, input_tensor_values.data(), input_tensor_size, input_shape.data(), input_shape.size());

        // Prepare input and output node names
        const char* input_names[] = {input_node_names[0].c_str()};
        const char* output_names[] = {output_node_names[0].c_str()};

        auto output_tensors = session.Run(
            Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);

        // Get output tensor
        float* output_data = output_tensors[0].GetTensorMutableData<float>();

        // Get output shape
        Ort::TensorTypeAndShapeInfo output_info = output_tensors[0].GetTensorTypeAndShapeInfo();
        std::vector<int64_t> output_shape = output_info.GetShape();

        // Output the shape and data
        std::cout << "Model output " << this->cnt + 1 << ": ";
        this->cnt++;
        for (auto s : output_shape) std::cout << s << " ";
        std::cout << std::endl;

        size_t output_size = output_info.GetElementCount();
        std::cout << "Output data: ";
        for (size_t i = 0; i < output_size; i++) {
            std::cout << output_data[i] << " ";
        }
        std::cout << std::endl;

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_this = end - start;
        this->elapsed += elapsed_this;
        this->processed_lines += 1;
    }

    void updateBuffer(const std::vector<float>& newVector) {
        if (buffer.size() >= capacity) {
            buffer.pop_front();  // Remove the oldest vector if we are at capacity
        }
        buffer.push_back(newVector);  // Add the new vector to the buffer
    }

    void processFile() {
        auto start = std::chrono::high_resolution_clock::now();

        // feedModel();          // Run the model on every new line
        while(true) {
            feedModel();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        std::cout << "\n\n\n";
        std::cout << "Elapsed time: " << this->elapsed.count() << " seconds." << std::endl;
        std::cout << "Processed " << this->processed_lines << " lines." << std::endl;
        std::cout << "Speed: " << this->processed_lines / this->elapsed.count() << " lines per second.\n\n" << std::endl;
    }

    void start(const std::string& filename = "") {
        this->cnt = 0;
        std::thread worker(&ModelRunner::processFile, this);
        worker.join();
    }
};

int main(int argc, char** argv) {
    argparse::ArgumentParser program("FAM Benchmarking Program");

    // // Add arguments
    // program.add_argument("-f", "--file_path")
    //     .help("Path to the CSV file containing feature records")
    //     .default_value(std::string("data/demo/feature_model/0.csv"));

    program.add_argument("-s", "--sequence_length")
        .help("sqeuence length")
        .default_value(40)
        .scan<'i', size_t>(); // Scanning as size_t; 

    program.add_argument("-m", "--model_path")
        .help("Path to the model file")
        .default_value(std::string("tra_pred_model/model/model_tft_vqvae_cpu_preproc.pt"));

    program.add_argument("--feature_dim")
        .help("Feature dimension")
        .default_value(32)
        .scan<'i', int>();

    program.add_argument("--log_dir")
        .help("Path to the directory containing log files")
        .default_value(std::string("logs"));

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << program;
        exit(0);
    }

    // Get the file path and buffer size from the arguments
    // std::string file_path = program.get<std::string>("-f");
    const size_t sequence_length = program.get<size_t>("-s");
    std::string model_path = program.get<std::string>("-m");
    int feature_dim = program.get<int>("--feature_dim");
    std::string log_dir = program.get<std::string>("--log_dir");
    // LogReader log_reader(log_dir);



    ModelRunner runner(model_path,
                       feature_dim, // Feature dimension
                       sequence_length, 
                       log_dir); // Capacity or batch size

    runner.start();
    return 0;
}

