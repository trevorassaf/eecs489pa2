#pragma once

#include "hash.h"

#include <stdint.h>
#include <string>
#include <assert.h>

#define MAX_DB_SIZE 1024
#define MAX_IMAGE_NAME 256

#define IMAGE_FOLDER "images/"
#define IMAGE_MANIFEST_FILE_NAME "FILELIST.txt"
#define IMAGE_MANIFEST_PATH IMAGE_FOLDER IMAGE_MANIFEST_FILE_NAME 

enum QueryResult {
  QUERY_SUCCESS,      // IMGDB_HIT
  BLOOM_FILTER_MISS,  // IMGDB_MISS
  QUERY_FAILURE       // IMGDB_FALSE
};

class ImageDb {

  private:
   /**
    * Specifies whether the db has been initialized or not. Should be 
    * initialized after we have know our initial jurisdiction of the identifier
    * ring.
    */
    bool isInitialized_;

    /**
     * Stores the id range for image db.
     */
    struct id_range_t {
      uint8_t start, end;   // (start, end]
    };

    id_range_t idRange_;

    /**
     * Bloom filter bits.
     */
    uint64_t bloomFilter_;

    /**
     * Tracks the number of images in the db.
     */
    uint16_t numImages_;

    /**
     * Represents an image in our database.
     */
    struct image_t {
      uint8_t id;
      std::string name;
    };

    /**
     * Tracks the images in our database.
     */
    image_t images_[MAX_DB_SIZE];

    /**
     * storeImage()
     * - Incorporate image into db.
     * @param id : id of image
     * @param md : sha1 hash of image name
     * @parm file_name : name of image file
     */
    void storeImage(uint8_t id, unsigned char * md, const std::string& file_name);

  public:

    /**
     * ImageDb()
     * - Ctor for image db. Starts w/everything in its db.
     */
    ImageDb(uint8_t id);

    /**
     * load()
     * - Clear db/bloomfilter and load contents based on current id-range.
     * @param start : beginning of new identifier ring (exclusive)
     * @param end : end of new identifier ring (inclusive)
     */
    void load(uint8_t start, uint8_t end);

    /**
     * cacheImage()
     * - Register the image with the cache. Only store name
     *   because directory contains all of the images. We just need
     *   to 'track' which images are recognized by this node.
     * @param file_name : name of image file to add to the cache.
     */
    void cacheImage(const std::string& file_name);

    /**
     * query()
     * - Query db for image.
     * @param file_name : name of image file
     * @return result of query
     */
    QueryResult query(const std::string& file_name) const; 
    
};
