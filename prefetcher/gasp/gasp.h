#include "svm4ap/global.hpp"

class GASP{
    constexpr static std::size_t INPUT_BUFFER_SETS = 256;
    constexpr static std::size_t INPUT_BUFFER_WAYS = 4;
    constexpr static std::size_t NUM_CLASSES = 4;
    constexpr static std::size_t SEQUENCE_SIZE = 4;
    constexpr static int PREFETCH_DEGREE = 3;

    constexpr static uint8_t confidenceThreshold = 12;
    constexpr static uint8_t maxConfidence = 15;

    shared_ptr<InputBuffer<ConfidenceInputBufferEntry>> inputBuffer;
    shared_ptr<Dictionary> dictionary;
    shared_ptr<SVM> svm ;

    struct lookahead_entry {
      uint64_t address = 0;
      int64_t stride = 0;
      int degree = 0; // degree remaining
    };

    std::optional<lookahead_entry> active_lookahead;

  public:

    GASP(){
        this->inputBuffer = shared_ptr<InputBuffer<ConfidenceInputBufferEntry>>((InputBuffer<ConfidenceInputBufferEntry>*) 
            new StandardInputBuffer<ConfidenceInputBufferEntry>(INPUT_BUFFER_SETS, INPUT_BUFFER_WAYS, SEQUENCE_SIZE, NUM_CLASSES));
        this->dictionary = shared_ptr<Dictionary>((Dictionary*)
            new OriginalDictionary(NUM_CLASSES));
        this->svm = shared_ptr<SVM>((SVM*)
            new StandardSVM(SEQUENCE_SIZE, NUM_CLASSES));

    }

    uint8_t incrementConfidence(uint8_t confidence){
      if(confidence >= maxConfidence) return confidence;
      else return confidence + 1;
    }

    uint8_t decrementConfidence(uint8_t confidence){
      if(confidence <= 0) return confidence;
      else return confidence - 1;
    }

    vector<double> adaptSequenceForSVM(vector<uint8_t>& sequence){
      vector<double> res = vector<double>();
      for (int j = 0; j < SEQUENCE_SIZE; j++) {
        res.push_back(((double)sequence[j]) / NUM_CLASSES + 1.0);
      }
      return res;
    }

    std::optional<uint64_t> predict(uint64_t ip, uint64_t addr){
      // 1) We read the input buffer entry:
      std::optional<ConfidenceInputBufferEntry> inputBufferEntry_ = 
        this->inputBuffer->read(ip);

      // If it hits, we proceed:
      if(inputBufferEntry_.has_value()){
        ConfidenceInputBufferEntry inputBufferEntry = inputBufferEntry_.value();
        int64_t delta = ((int64_t)addr) - ((int64_t)inputBufferEntry.lastAddress);

        // 2) We update the dictionary and the sequence:
        auto class_ = dictionary->write(delta);
        vector<uint8_t> sequence = inputBufferEntry.classSequence;
        for(int i = 1; i < sequence.size(); i++)
          sequence[i-1] = sequence[i];
        sequence[sequence.size()-1] = class_;

        // 3) We evaluate the prediction hit or miss:
        auto predictedClass = inputBufferEntry.predictedClass;
        auto confidence = inputBufferEntry.confidence;
        if(predictedClass == class_){
          confidence = incrementConfidence(confidence);
          auto sequence_ = adaptSequenceForSVM(sequence);
          auto newPredictedClass = svm->predict(sequence_);
          ConfidenceInputBufferEntry newInputBufferEntry = {
            ip,
            addr, 
            sequence,
            newPredictedClass,
            confidence
          };
          if(confidence >= confidenceThreshold)
            return addr + dictionary->read(newPredictedClass).value();
          this->inputBuffer->write(newInputBufferEntry);
        }
        else {
          uint8_t newPredictedClass;
          if(predictedClass != NUM_CLASSES){
            confidence = decrementConfidence(confidence);
            auto sequence_ = adaptSequenceForSVM(inputBufferEntry.classSequence);
            svm->fit(sequence_, class_);
            newPredictedClass = NUM_CLASSES;
          }
          else{
            auto sequence_ = adaptSequenceForSVM(sequence);
            newPredictedClass = svm->predict(sequence_);
            if(confidence >= confidenceThreshold)
              return addr + dictionary->read(newPredictedClass).value();
          }
          ConfidenceInputBufferEntry newInputBufferEntry = {
            ip,
            addr, 
            sequence,
            newPredictedClass,
            confidence
          };
          this->inputBuffer->write(newInputBufferEntry);
        }
      }
      else{
        ConfidenceInputBufferEntry inputBufferEntry = {
            ip,
            addr, 
            vector<uint8_t>(NUM_CLASSES, SEQUENCE_SIZE),
            NUM_CLASSES,
            0
        };
        this->inputBuffer->write(inputBufferEntry);
      }
      
      return nullopt;
    }

    void initiate_lookahead(uint64_t ip, uint64_t cl_addr)
    {
      auto predictedAddress = this->predict(ip, cl_addr);
      int64_t stride = 0;
      if(predictedAddress.has_value()){
        // calculate the stride between the current address and the last address
        // no need to check for overflow since these values are downshifted
        stride = static_cast<int64_t>(cl_addr) - static_cast<int64_t>(predictedAddress.value());

        // Initialize prefetch state unless we somehow saw the same address twice in
        // a row or if this is the first time we've seen this stride
        if (stride != 0)
          active_lookahead = {cl_addr << LOG2_BLOCK_SIZE, stride, PREFETCH_DEGREE};
      }

    }

    void advance_lookahead(CACHE* cache)
    {
      // If a lookahead is active
      if (active_lookahead.has_value()) {
        auto [old_pf_address, stride, degree] = active_lookahead.value();
        assert(degree > 0);

        auto addr_delta = stride * BLOCK_SIZE;
        auto pf_address = static_cast<uint64_t>(static_cast<int64_t>(old_pf_address) + addr_delta); // cast to signed to allow negative strides

        // If the next step would exceed the degree or run off the page, stop
        if (cache->virtual_prefetch || (pf_address >> LOG2_PAGE_SIZE) == (old_pf_address >> LOG2_PAGE_SIZE)) {
          // check the MSHR occupancy to decide if we're going to prefetch to this level or not
          bool success = cache->prefetch_line(pf_address, (cache->get_mshr_occupancy_ratio() < 0.5), 0);
          if (success)
            active_lookahead = {pf_address, stride, degree - 1};
          // If we fail, try again next cycle

          if (active_lookahead->degree == 0) {
            active_lookahead.reset();
          }
        } else {
          active_lookahead.reset();
        }
      }
    }
  };