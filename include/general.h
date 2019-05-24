static const char *productId = "100002151";  //100002151
static const char *aliasKey = "test";
static const char *deviceProfile = "AbgzjoW/eKHY0abI1P16xfxbdImrLfohvlQ4uPJkX5IovJ7fWOWckoTdm5qJlpyarJqcjZqL3cXdzMfOz5manMqbnMrGy5qczMfHm87PzcabmZ7Om8/OzM/d092ek5OQiN3FztPdj42Qm4qci7ab3cXdzs/Pz8/NzsrO3dPdm5qJlpyasZ6Smt3F3ZnPzcfJms+czM/InsvIx8adxsjNmcedm57Pyc6aycfH3dPdjJyQj5rdxaTdnpOT3aKC";
static const char *savedProfile = "example_general/100002151.txt";
static const char *productKey = "xxx";
static const char *productSecret = "yyy";

enum _status{
	DDS_STATUS_NONE = 0,
	DDS_STATUS_IDLE,
	DDS_STATUS_LISTENING,
	DDS_STATUS_UNDERSTANDING
};

enum _status dds_status;