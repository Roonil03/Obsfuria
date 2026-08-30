# How to Contribute

See [CONTRIBUTING.md](./CONTRIBUTING.md) for full contribution guidelines.

Quick summary:
1. Fork the repository
2. Create feature branch: `git checkout -b feature/YourFeature`
3. Build: `bash ./scripts/add_commands.sh && make`
4. Test: `make test`
5. Commit & push
6. Open Pull Request

**Key rules for new enemies:**
- Use non-alphanumeric or C-error style names (e.g., "Segmentation Fault")
- Add to `assets/default_commands.txt` or `assets/crazy_commands.txt` as `name|command`
- No alphanumeric-only names
