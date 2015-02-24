#include "ImageDb.h"

#include <iostream>
#include <fstream>

ImageDb::ImageDb(uint8_t id) : 
  isInitialized_(false),
  idRange_{id, id}
{
  load(id, id);  
}

void ImageDb::load(uint8_t start, uint8_t end) {

  // We are now in the 'initialized' state
  isInitialized_ = true;
  
  // Store the new bounds of the identifier ring
  idRange_ = {start, end};

  // Clear current data
  bloomFilter_ = 0;
  numImages_ = 0;

  // Report that we're loading the db with images in our range
  std::cout << "IMGDB: Loading ImageDb with images in range: (" << (int) idRange_.start <<
      ", " << (int) idRange_.end << "]" << std::endl;

  // Load images
  std::ifstream manifest(IMAGE_MANIFEST_PATH);
  std::string file_name;

  while (manifest >> file_name && numImages_ < MAX_DB_SIZE) {
    unsigned char md[SHA1_MDLEN];

    SHA1((unsigned char *) file_name.c_str(), file_name.size(), md);
    uint8_t id = static_cast<uint8_t>(ID(md));

    if (ID_inrange(id, idRange_.start, idRange_.end)) {
      storeImage(id, md, file_name);
    }
  }

  manifest.close();
}

void ImageDb::storeImage(
  uint8_t id,
  unsigned char * md,
  const std::string& file_name
) {
  // Fail if the image db has not yet been initialized
  assert(isInitialized_);

  // Check that image can be loaded from file system
  std::string image_path = IMAGE_FOLDER + file_name;
  std::ifstream image_file(image_path);

  // Fail if image can't be loaded
  assert(!image_file.fail());

  image_file.close();

  // Store image info in db
  image_t& image = images_[numImages_];
  image.id = id;
  image.name = file_name;

  // Report that we'res storing a new image
  std::cout << "Storing new image in db: <id: " << (int) image.id << ", name: " <<
      image.name << ">" << std::endl;

  // Update bloom filter
  bloomFilter_ |= (1L << (int) bfIDX(BFIDX1, md)) |
      (1L << (int) bfIDX(BFIDX2, md)) | (1L << (int) bfIDX(BFIDX3, md));

  // Track the newly added image
  ++numImages_;
}

void ImageDb::cacheImage(const std::string& file_name) {
  
}

QueryResult ImageDb::query(const std::string& file_name) const {
  return QUERY_SUCCESS;
}
