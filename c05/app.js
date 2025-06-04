// app.js
const express = require('express');
const { testDbConnectionAndSetup } = require('./db'); // Import the combined function
// const usersRoutes = require('./routes/users'); // Comment out or remove users routes
const blobsRoutes = require('./routes/blobs'); // Import blobs routes

const app = express();
const port = process.env.PORT || 3001;

// Configure express to handle JSON and increase payload size limit
app.use(express.json({ limit: '50mb' }));

app.use((req, res, next) => {
  console.log(`${new Date().toISOString()} - ${req.method} ${req.originalUrl}`);
  next();
});

// app.use('/api/users', usersRoutes); // Comment out or remove users routes
app.use('/api/blobs', blobsRoutes); // Use blobs routes

app.get('/', (req, res) => {
  res.send('Welcome to the Node.js Express MySQL API!');
});

// Start the server with DB setup
async function startServer() {
  await testDbConnectionAndSetup(); // Call the function that connects AND creates tables
  app.listen(port, () => {
    console.log(`Server listening on port ${port}`);
    console.log(`Access at http://localhost:${port}`);
    // console.log(`User endpoints: http://localhost:${port}/api/users`); // Comment out or remove user endpoints log
    console.log(`Blob endpoints: http://localhost:${port}/api/blobs`); // Log blob endpoints
  });
}

startServer();