#ifndef ENUM_FLAG_H_
#define ENUM_FLAG_H_

// Support to make a enum into an Absl Flag.
template <class EnumType, EnumType FinalEnum>
class EnumFlag {
 public:
  std::vector<EnumType> All(); 
};

#endif  // ENUM_FLAG_H_
