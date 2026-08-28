# Deploy to Netlify

## Quick Deploy

1. Push this repo to GitHub/GitLab/Bitbucket
2. Go to [app.netlify.com](https://app.netlify.com)
3. Click "Add new site" → "Import an existing project"
4. Select your repository
5. Configure:
   - **Base directory:** (leave empty)
   - **Build command:** `echo "static site"`
   - **Publish directory:** `webui`
6. Click "Deploy site"

## Local Development

```bash
# Install Netlify CLI
npm install -g netlify-cli

# Run locally
netlify dev

# This starts the site with functions at http://localhost:8888
```

## How It Works

- **Frontend:** `webui/index.html` - Static HTML/CSS/JS dashboard
- **Backend:** `netlify/functions/` - Serverless functions for API
  - `state.mjs` - Returns cluster state (mock data for demo)
  - `command.mjs` - Handles commands (add-worker, status, etc.)

The dashboard works in two modes:
1. **Local:** Connects to your Python server at `localhost:8080`
2. **Netlify:** Uses serverless functions with mock data for demo

## API Endpoints

On Netlify, the frontend calls:
- `GET /.netlify/functions/state` - Get cluster state
- `POST /.netlify/functions/command` - Execute commands

Request body for commands:
```json
{
  "command": "add-worker",
  "name": "my-worker"
}
```

Available commands:
- `status` - Show cluster status
- `add-worker` - Add a dummy worker
- `random-worker` - Add a random worker
- `remove-worker` - Remove a dummy worker
- `clear-workers` - Clear all dummy workers
- `submit` - Submit a workload
- `inspect` - Inspect a chunk
- `cancel` - Cancel a chunk
- `drain` - Drain a worker
