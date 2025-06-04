// db.js
const mysql = require('mysql2/promise');
require('dotenv').config({ path: 'local.env' }); // Load environment variables from local.env

console.log('Creating MySQL pool with config:', {
  host: process.env.DB_HOST,
  user: process.env.DB_USER,
  // password: process.env.DB_PASSWORD, // Do NOT log sensitive info!
  database: process.env.DB_DATABASE,
  port: process.env.DB_PORT,
  waitForConnections: true,
  connectionLimit: 10,
  queueLimit: 0
});

let pool = mysql.createPool({
  host: process.env.DB_HOST,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  database: process.env.DB_DATABASE, // Ensure this database already exists or is created by Docker init script
  port: process.env.DB_PORT,
  waitForConnections: true,
  connectionLimit: 10,
  queueLimit: 0
});

async function createDatabaseIfNotExists() {
  // Connect without specifying database
  const connection = await mysql.createConnection({
    host: process.env.DB_HOST,
    user: process.env.DB_USER,
    password: process.env.DB_PASSWORD,
    port: process.env.DB_PORT
  });
  const dbName = process.env.DB_DATABASE;
  await connection.query(`CREATE DATABASE IF NOT EXISTS \`${dbName}\``);
  console.log(`Database '${dbName}' ensured to exist.`);
  await connection.end();
}

async function createTables() {
  try {
    const connection = await pool.getConnection(); // Get a connection from the pool

    // Create image_blobs table
    const createImageBlobsTableSQL = `
      CREATE TABLE IF NOT EXISTS image_blobs (
        id INT AUTO_INCREMENT PRIMARY KEY,
        file_name VARCHAR(255) NOT NULL,
        uuid VARCHAR(36) UNIQUE NOT NULL,
        image_data LONGBLOB NOT NULL,
        content_type VARCHAR(100) NOT NULL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
      );
    `;
    await connection.execute(createImageBlobsTableSQL);
    console.log('Table "image_blobs" ensured to exist.');

    // You can add more tables here if needed
    // const createProductsTableSQL = `
    //   CREATE TABLE IF NOT EXISTS products (
    //     id INT AUTO_INCREMENT PRIMARY KEY,
    //     name VARCHAR(255) NOT NULL,
    //     price DECIMAL(10, 2) NOT NULL
    //   );
    // `;
    // await connection.execute(createProductsTableSQL);
    // console.log('Table "products" ensured to exist.');

    connection.release(); // Release the connection back to the pool
    console.log('Database tables check/creation complete.');

  } catch (error) {
    console.error('Error during database table creation/check:', error.message);
    // It's critical to exit if schema setup fails, as the app won't function correctly
    process.exit(1);
  }
}

async function testDbConnectionAndSetup() {
  try {
    // Try to get a connection; if database doesn't exist, catch and create it
    try {
      const connection = await pool.getConnection();
      console.log('Successfully connected to the MySQL database!');
      connection.release(); // Release the connection back to the pool
    } catch (error) {
      if (error.code === 'ER_BAD_DB_ERROR' || error.message.includes('Unknown database')) {
        console.warn(`Database '${process.env.DB_DATABASE}' does not exist. Creating...`);
        await createDatabaseIfNotExists();
        // Recreate the pool now that the DB exists
        pool = mysql.createPool({
          host: process.env.DB_HOST,
          user: process.env.DB_USER,
          password: process.env.DB_PASSWORD,
          database: process.env.DB_DATABASE,
          port: process.env.DB_PORT,
          waitForConnections: true,
          connectionLimit: 10,
          queueLimit: 0
        });
        const connection = await pool.getConnection();
        console.log('Successfully connected to the MySQL database after creation!');
        connection.release();
      } else {
        throw error;
      }
    }
    // Now call createTables after successful connection
    await createTables();
  } catch (error) {
    console.error('Error connecting to or setting up the database:', error.message);
    process.exit(1);
  }
}

module.exports = {
  pool,
  testDbConnectionAndSetup // Export the combined function
};