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
  std::cout << "\t- Loading database with images in range: (" << (int) idRange_.start <<
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
  std::cout << "\t\t- Storing new image in db: <id: " << (int) image.id << ", name: " <<
      image.name << ", idx: " << (int) numImages_ << ">" << std::endl;

  // Update bloom filter
  bloomFilter_ |= (1L << (int) bfIDX(BFIDX1, md)) |
      (1L << (int) bfIDX(BFIDX2, md)) | (1L << (int) bfIDX(BFIDX3, md));
  
  // Track the newly added image
  ++numImages_;
}

void ImageDb::cacheImage(const std::string& file_name) {
  // Report that we're trying to cache the image
  std::cout << "\t- Attempting to cache image..." << std::endl;
 
  // Compute SHA1 hash and id of image name
  unsigned char md[SHA1_MDLEN];
  SHA1((unsigned char *) file_name.c_str(), file_name.size(), md);
  uint8_t id = static_cast<uint8_t>(ID(md));

  // Insert into db, if there's room
  if (numImages_ != MAX_DB_SIZE) {
    // Add image to the cache
    storeImage(id, md, file_name);
    
    // Report that we've cached the image
    std::cout << "\t- Successfully cached image!" << std::endl;

  } else {
    // Report that the cache is full
    std::cout << "\t- Couldn't cache image because db is full!" << std::endl;
  }
}

QueryResult ImageDb::query(const std::string& file_name) const {
  
  // Compute SHA1 and id
  unsigned char md[SHA1_MDLEN];
  SHA1((unsigned char *) file_name.c_str(), file_name.size(), md);
  uint8_t id = ID(md);

  if (!((bloomFilter_ & (1L << (int) bfIDX(BFIDX1, md))) &&
      (bloomFilter_ & (1L << (int) bfIDX(BFIDX2, md))) &&
      (bloomFilter_ & (1L << (int) bfIDX(BFIDX3, md)))))
  { 
    return QUERY_FAILURE;
  }

  
  /* To get here means that you've got a hit at the Bloom Filter.
   * Search the DB for a match to BOTH the image ID and name.
  */
  for (size_t i = 0; i < numImages_; ++i) {
    const image_t& image = images_[i];
    if (id == image.id && file_name == image.name) {
      return QUERY_SUCCESS;
    }
  }

  return BLOOM_FILTER_MISS; 
}
