// routes/blobs.js
const express = require('express');
const router = express.Router();
const { pool } = require('../db'); // Import the database pool

// Middleware to parse raw binary data
router.use(express.raw({
  type: ['application/octet-stream', 'image/*'],
  limit: '10mb' // Adjust limit as needed
}));

// GET all image blobs (metadata only)
router.get('/', async (req, res) => {
  try {
    const [rows] = await pool.execute('SELECT id, file_name, uuid, content_type, created_at FROM image_blobs');
    res.json(rows);
  } catch (error) {
    console.error('Error fetching image blobs:', error);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

// GET file directly by UUID
router.get('/:uuid/file', async (req, res) => {
  const { uuid } = req.params;
  
  try {
    // Get the image file data
    const [rows] = await pool.execute(
      'SELECT file_name, image_data, content_type FROM image_blobs WHERE uuid = ?', 
      [uuid]
    );
    
    if (rows.length === 0) {
      return res.status(404).json({ error: 'Image file not found' });
    }
    
    const blob = rows[0];
    
    // Set appropriate headers for file download
    res.setHeader('Content-Type', blob.content_type);
    res.setHeader('Content-Disposition', `attachment; filename="${blob.file_name}"`);
    res.setHeader('Content-Length', blob.image_data.length);
    
    // Send the file data
    return res.send(blob.image_data);
  } catch (error) {
    console.error(`Error serving file with UUID ${uuid}:`, error);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

// GET image blob by UUID
router.get('/:uuid', async (req, res) => {
  const { uuid } = req.params;
  const metadataOnly = req.query.metadata === 'true';
  
  try {
    let query, rows;
    
    if (metadataOnly) {
      // Return metadata only
      [rows] = await pool.execute('SELECT id, file_name, uuid, content_type, created_at FROM image_blobs WHERE uuid = ?', [uuid]);
      if (rows.length === 0) {
        return res.status(404).json({ error: 'Image blob not found' });
      }
      return res.json(rows[0]);
    } else {
      // Return full blob including image data
      [rows] = await pool.execute('SELECT id, file_name, uuid, image_data, content_type, created_at FROM image_blobs WHERE uuid = ?', [uuid]);
      if (rows.length === 0) {
        return res.status(404).json({ error: 'Image blob not found' });
      }
      
      // Send image data as binary response
      const blob = rows[0];
      res.setHeader('Content-Type', blob.content_type);
      res.setHeader('Content-Disposition', `inline; filename="${blob.file_name}"`);
      return res.send(blob.image_data);
    }
  } catch (error) {
    console.error(`Error fetching image blob with UUID ${uuid}:`, error);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

// POST new image blob
router.post('/', express.json(), async (req, res) => {
  // This route can handle two formats:
  // 1. JSON with file_name, content_type, uuid, and image_data as base64
  // 2. Raw binary with headers for file_name, content_type, and uuid
  
  let file_name, content_type, uuid, imageData;

  // Log the whole request body for debugging
  console.log('Request body:', req.body);
  // Log the request headers for debugging
  console.log('Request headers:', req.headers);
  // Check the content type of the request
  console.log('Request content type:', req.headers['content-type']);
  // Determine the format of the request
  if (!req.is('application/json') && !req.is(['application/octet-stream', 'image/*'])) {
    return res.status(415).json({ error: 'Unsupported media type' });
  }

  
  if (req.is('application/json')) {
    // JSON format
    file_name = req.body.file_name;
    content_type = req.body.content_type;
    uuid = req.body.uuid;
    
    // Check if image_data is provided as base64 or raw bytes
    if (req.body.image_data) {
      if (typeof req.body.image_data === 'string') {
        try {
          imageData = Buffer.from(req.body.image_data, 'base64');
        } catch (e) {
          return res.status(400).json({ error: 'Invalid base64 image data' });
        }
      } else if (Buffer.isBuffer(req.body.image_data)) {
        imageData = req.body.image_data;
      } else {
        return res.status(400).json({ error: 'Invalid image data format' });
      }
    }
  } else if (req.is(['application/octet-stream', 'image/*'])) {
    // Raw binary format with headers
    file_name = req.headers['x-file-name'];
    content_type = req.headers['content-type'];
    uuid = req.headers['x-uuid'];
    imageData = req.body; // Raw binary data
  } else {
    return res.status(415).json({ error: 'Unsupported media type' });
  }
  
  // Validate required fields
  if (!file_name) {
    return res.status(400).json({ error: 'file_name is required (in body or as X-File-Name header)' });
  }
  
  if (!content_type) {
    return res.status(400).json({ error: 'content_type is required (in body or as Content-Type header)' });
  }
  
  if (!uuid) {
    return res.status(400).json({ error: 'uuid is required (in body or as X-UUID header)' });
  }
  
  if (!imageData || Buffer.byteLength(imageData) === 0) {
    return res.status(400).json({ error: 'Image data is required' });
  }
  
  try {
    // Insert with the provided UUID
    const [insertResult] = await pool.execute(
      'INSERT INTO image_blobs (file_name, uuid, image_data, content_type) VALUES (?, ?, ?, ?)',
      [file_name, uuid, imageData, content_type]
    );
    
    res.status(201).json({ uuid: uuid }); // Return the provided uuid
  } catch (error) {
    console.error('Error creating image blob:', error);
    if (error.code === 'ER_DUP_ENTRY') {
      return res.status(409).json({ error: 'UUID already exists or duplicate entry.' });
    }
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

// PUT update image blob by UUID
router.put('/:uuid', express.json(), async (req, res) => {
  const { uuid } = req.params;
  let file_name, content_type, imageData;
  
  if (req.is('application/json')) {
    // JSON format
    file_name = req.body.file_name;
    content_type = req.body.content_type;
    
    // Check if image_data is provided as base64 or raw bytes
    if (req.body.image_data) {
      if (typeof req.body.image_data === 'string') {
        try {
          imageData = Buffer.from(req.body.image_data, 'base64');
        } catch (e) {
          return res.status(400).json({ error: 'Invalid base64 image data' });
        }
      } else if (Buffer.isBuffer(req.body.image_data)) {
        imageData = req.body.image_data;
      } else {
        return res.status(400).json({ error: 'Invalid image data format' });
      }
    }
  } else if (req.is(['application/octet-stream', 'image/*'])) {
    // Raw binary format with headers
    file_name = req.headers['x-file-name'];
    content_type = req.headers['content-type'];
    imageData = req.body; // Raw binary data
  }
  
  if (!file_name && !imageData && !content_type) {
    return res.status(400).json({ error: 'At least one field (file_name, image_data, or content_type) is required for update' });
  }
  
  try {
    let updateQuery = 'UPDATE image_blobs SET ';
    const updateParams = [];
    
    if (file_name) {
      updateQuery += 'file_name = ?, ';
      updateParams.push(file_name);
    }
    
    if (content_type) {
      updateQuery += 'content_type = ?, ';
      updateParams.push(content_type);
    }
    
    if (imageData) {
      updateQuery += 'image_data = ?, ';
      updateParams.push(imageData);
    }
    
    // Remove trailing comma and space
    updateQuery = updateQuery.slice(0, -2);
    updateQuery += ' WHERE uuid = ?';
    updateParams.push(uuid);
    
    const [result] = await pool.execute(updateQuery, updateParams);
    
    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'Image blob not found' });
    }
    
    // Fetch the updated blob to return it (without the binary data)
    const [updatedRows] = await pool.execute(
      'SELECT id, file_name, uuid, content_type, created_at FROM image_blobs WHERE uuid = ?', 
      [uuid]
    );
    
    res.json({ 
      message: 'Image blob updated successfully', 
      blob: updatedRows[0] 
    });
  } catch (error) {
    console.error(`Error updating image blob with UUID ${uuid}:`, error);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

// DELETE image blob by UUID
router.delete('/:uuid', async (req, res) => {
  const { uuid } = req.params;
  try {
    const [result] = await pool.execute('DELETE FROM image_blobs WHERE uuid = ?', [uuid]);
    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'Image blob not found' });
    }
    res.json({ message: 'Image blob deleted successfully', uuid });
  } catch (error) {
    console.error(`Error deleting image blob with UUID ${uuid}:`, error);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

module.exports = router;
