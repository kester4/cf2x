<div align="center">
  <img src="https://github.com/kester4/cf2x/blob/main/assets/icon.png?raw=true"
       alt="cf2x Icon"
       width="128"
       height="128">
  <p>A simple function plotter written in C and raylib</p>
</div>

### Features
- Supports single-variable equations with brackets, math operans and trigonometric functions: `sin`, `cos`, `tan`, `cot`
- Is able to parse implicit multiplication and unary negation (shunting yard parser with RPN evaluator)
- Uses recursive adaptive sampling to handle\* curvature plots
- Has interactive pan and zoom, dynamic coordinate grid and axis labels
- Supports window resizing and SSAA for smoother lines
- Render caching doesn't redraw canvas on every frame

### Current issues
- \*Discontinuities such as `1/x + C`, where `C != 0` or `2.71^x`, where `x > 0` flattens out past some zoom-out level (adaptive sampling issue)
- No input panel vertical scrolling, so inputs can go past screen if there are too many
- Memory leakes and crashes are possible, some equations may produce unexpected results

### Installation
1) Install [Raylib](https://github.com/raysan5/raylib)
2) Run:
```bash
git clone https://github.com/kester4/cf2x.git && cd cf2x
```
```bash
make
```
```bash
make run
```

### Controls
- `Enter` on the lowest equation to add a new one 
- `Ctrl + R` to reset zoom and pan to defaults
- `R` to reset pan
- `Up`, `Down`, `Left`, `Right` for input menu navigation

### TODO
- [ ] Add ~~horizontal~~/vertical scrolling to the input menu  
- [ ] Make `refine_plot()` detect things like `1/x + C`  
- [x] Implement inserting new equations between already existing ones in the input menu
- [ ] Add `log`/`ln` and `exp` letter support  
- [ ] Offload plots rendering to the GPU  
- [ ] Add a dark theme