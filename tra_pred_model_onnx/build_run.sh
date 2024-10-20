# Build the main.cpp file with the onnxruntime library
g++ -v -std=c++17 \
    -I/Users/shawn/Documents/UMSI/Boeing_Project/onnxruntime/include \
    main.cpp \
    /Users/shawn/Documents/UMSI/Boeing_Project/onnxruntime/build/MacOS/Release/libonnxruntime.dylib \
    -o main \
    -Wl,-rpath,/Users/shawn/Documents/UMSI/Boeing_Project/onnxruntime/build/MacOS/Release/


# run the main file
./main \
    --file_path data/demo/feature_model/0.csv \
    --batch_size 40 \
    --model_path model/model.onnx \
    --feature_dim 32


