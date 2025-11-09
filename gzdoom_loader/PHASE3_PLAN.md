# GZDoom Loader - Phase 3 Implementation Plan

## GZDoom Multiplayer Support Overview

### Available Parameters
Based on GZDoom documentation:

**Host Game:**
- `-host N` - Host a game with N players (max 8)
- Default mode: Cooperative
- Optional: `-deathmatch` for deathmatch mode

**Join Game:**
- `-join IP_ADDRESS` - Join a hosted game
- Default port: 5029
- Inherits IWAD/PWADs from command line

**Network Options:**
- `-netmode 0` - Peer-to-peer (default, faster, requires open ports)
- `-netmode 1` - Packet server (allows firewalled clients if host is not firewalled)
- `-port N` - Custom port (default 5029)
- `-dup N` - Network performance tuning (1-5, default 1)

**Game Modes:**
- Default: Cooperative
- `-deathmatch` - Deathmatch mode
- `-altdeath` - Alternative deathmatch scoring

### Example Commands

**Host cooperative game:**
```bash
gzdoom -iwad doom2.wad -host 4 -warp 01 -skill 3
```

**Join game:**
```bash
gzdoom -iwad doom2.wad -file mod.pk3 -join 192.168.1.100
```

**Host deathmatch:**
```bash
gzdoom -iwad doom2.wad -host 8 -deathmatch -warp 01
```

## Proposed Implementation

### Feature 1: Multiplayer Panel 🎮

**UI Components:**
```
[Multiplayer Mode]
  ( ) Single Player (default)
  ( ) Host Game
  ( ) Join Game

[When "Host Game" selected:]
  Players: [2] (spinner: 2-8)
  Game Mode: [Cooperative ▼] (Cooperative/Deathmatch/AltDeath)
  Network Mode: [Peer-to-Peer ▼] (Peer-to-Peer/Packet Server)
  Port: [5029]

[When "Join Game" selected:]
  Server IP: [________]
  Port: [5029]
```

**Implementation Details:**
- Add checkbox group for mode selection
- Conditional visibility for host/join options
- Validation: IP address format, port range
- Command generation includes appropriate flags

### Feature 2: Recent Configurations 📋

**UI Components:**
```
[Recent Configurations] (5 most recent)
  - DOOM2 + Brutal Doom (Skill 4, MAP01)
  - Heretic + Custom Maps
  - DOOM + Project Brutality
  [Load] [Clear History]
```

**Implementation:**
- Track last 5-10 launched configurations
- Store in config file with timestamp
- Quick-load button for each entry
- Auto-populate all fields when selected

### Feature 3: Enhanced Preset Management 🏷️

**Current Limitation:** Auto-generated names like "Preset 1"

**Improvement:**
- Text input for custom preset name
- Multi-line description field
- Tags/categories (e.g., "Gameplay Mod", "Map Pack")
- Timestamp tracking
- Import/Export presets to JSON

**UI Enhancement:**
```
[Save New Preset]
  Name: [____________]
  Description: [____________]
           [____________]
  Category: [Gameplay Mod ▼]
  [Save] [Cancel]
```

### Feature 4: Multiple Source Port Support 🎯

**Supported Ports:**
- GZDoom (current)
- Zandronum (multiplayer-focused)
- LZDoom (legacy hardware)
- Crispy Doom (vanilla+)
- DSDA-Doom (demos)
- Chocolate Doom (pure vanilla)

**Implementation:**
```
[Source Port]
  Executable: [gzdoom]
  Type: [GZDoom ▼] (dropdown)
  [Auto-Detect All Ports]

[Detected Ports] (list)
  ✓ GZDoom 4.11.0 - /usr/bin/gzdoom
  ✓ Zandronum 3.1 - /usr/games/zandronum
  ✓ LZDoom 3.88 - /usr/bin/lzdoom
```

**Per-Port Features:**
- Different parameter sets
- Compatibility warnings
- Feature availability (e.g., Zandronum has better multiplayer)

### Feature 5: Other Quick Wins

**A. Keyboard Shortcuts:**
- Ctrl+O: Browse IWAD
- Ctrl+M: Add PWADs
- Ctrl+L: Launch
- Ctrl+S: Save preset
- F5: Auto-detect all

**B. Demo Recording:**
- `-record demoname` - Record demo
- `-playdemo demoname` - Play demo
- UI: Checkbox "Record Demo" + name field

**C. Extra Game Options:**
- `-respawn` - Monsters respawn
- `-fast` - Fast monsters
- `-nomonsters` - No monsters (practice)
- `-turbo N` - Speed multiplier (default 100)

**D. Advanced Options Tab:**
- Video settings (-width, -height, -fullscreen)
- Audio settings (-nosfx, -nomusic)
- Compatibility flags
- Console variables (+set commands)

## Implementation Priority

### High Priority (Implement Now)
1. **Multiplayer Support** ⭐⭐⭐
   - Most requested feature
   - Well-defined parameters
   - ~200 lines of code
   - Adds significant value

2. **Recent Configurations** ⭐⭐⭐
   - Improves UX dramatically
   - Simple implementation
   - ~100 lines of code

3. **Preset Naming** ⭐⭐
   - Addresses current limitation
   - User-requested
   - ~150 lines of code

### Medium Priority (Phase 4)
4. **Multiple Source Ports** ⭐⭐
   - Advanced users
   - Complex implementation
   - Needs port detection logic

5. **Demo Recording** ⭐
   - Niche feature
   - Simple to add
   - Low complexity

6. **Keyboard Shortcuts** ⭐
   - Nice-to-have
   - Requires event handling

### Low Priority (Future)
7. **Advanced Options Tab**
   - For power users only
   - UI complexity increase
   - Better as separate dialog

## Recommended Implementation Order

**Phase 3A: Multiplayer (Primary Focus)**
1. Add multiplayer UI panel
2. Implement host/join radio buttons
3. Add conditional fields (players, IP, etc.)
4. Update command generation
5. Add validation
6. Test with actual multiplayer session

**Phase 3B: UX Improvements**
1. Implement recent configurations
2. Add preset naming dialog
3. Track usage timestamps

**Phase 3C: Additional Features**
1. Multiple source port detection
2. Demo recording options
3. Keyboard shortcuts

## Technical Considerations

### UI Layout Expansion
Current window: 800x700
Proposed: 900x750 (accommodate multiplayer panel)

### Config File Format
Add sections:
```
MULTIPLAYER_MODE=single
HOST_PLAYERS=2
HOST_GAMEMODE=coop
JOIN_IP=192.168.1.100
NETWORK_PORT=5029

RECENT_COUNT=3
RECENT_0_IWAD=/path/to/doom2.wad
RECENT_0_TIMESTAMP=2025-01-15
...
```

### Validation Requirements
- IP address format validation
- Port range (1024-65535)
- Player count (2-8)
- Network mode compatibility checks

## User Approval Required

**Questions for you:**

1. **Priority**: Should we implement all of Phase 3A (multiplayer) first, or pick features from different phases?

2. **Multiplayer Scope**: Do you want:
   - [ ] Basic host/join support?
   - [ ] Full options (netmode, port, dup)?
   - [ ] Game mode selection (co-op/deathmatch)?

3. **Additional Features**: Which of these interest you most?
   - [ ] Recent configurations
   - [ ] Preset naming
   - [ ] Multiple source ports
   - [ ] Demo recording
   - [ ] Keyboard shortcuts

4. **Testing**: Do you have access to test multiplayer (requires 2+ machines or VM)?

Let me know your preferences and I'll start implementing!
