# Mobile Mining for PARTHENON CHAIN

## Overview

The PARTHENON CHAIN mobile wallet includes a **specialized mobile mining implementation** that is fundamentally different from the PC miners (CPU/GPU/CUDA/OpenCL).

**IMPORTANT:** Mobile mining uses a completely separate codebase and approach designed specifically for mobile devices. Do not attempt to use PC mining software on mobile devices.

## Key Differences from PC Mining

### PC Miners (CPU/GPU/CUDA/OpenCL)
- **Multi-threaded**: Uses 4-32+ CPU threads or GPU cores
- **Continuous hashing**: No sleep intervals, maximum throughput
- **Large batches**: 1024-4096+ hashes per batch
- **x86 optimizations**: AVX2, SIMD instructions
- **GPU acceleration**: CUDA/OpenCL for parallel processing
- **High power consumption**: Designed for plugged-in desktop/server systems
- **No battery awareness**: Runs at full power continuously

### Mobile Miner (This Implementation)
- **Single/minimal threading**: 1-2 threads to avoid overheating
- **Intermittent hashing**: Sleep intervals between batches
- **Small batches**: 10-100 hashes per batch
- **ARM optimizations**: Optimized for mobile processors
- **No GPU acceleration**: Uses mobile CPU only
- **Battery-aware**: Monitors battery level and charging state
- **Temperature monitoring**: Auto-stops when device gets hot
- **Background throttling**: Reduces mining when app is backgrounded

## Features

### Battery Management
- **Auto-stop on low battery**: Stops mining below configurable threshold (default: 30%)
- **Charging-only mode**: Can be configured to mine only when plugged in
- **Battery drain limit**: Configurable max battery drain per hour
- **Smart power management**: Adjusts hash rate based on battery level

### Thermal Management
- **Temperature monitoring**: Continuously monitors device temperature
- **Auto-stop on overheat**: Stops mining if temperature exceeds threshold (default: 40°C)
- **Thermal throttling**: Reduces hash rate as temperature increases
- **Cooling periods**: Built-in sleep intervals prevent overheating

### Background Behavior
- **Foreground mode**: Normal hash rate when app is in foreground
- **Background mode**: Dramatically reduced hash rate when backgrounded
- **Configurable batches**: Different batch sizes for foreground/background
- **Battery preservation**: Minimal background activity to save battery

## Configuration

Mobile mining is configured via `mobile_miner_config.example.json`:

```json
{
  "enabled": false,
  "poolUrl": "stratum+tcp://mobile.pool.drachma.org:3334",
  "workerName": "mobile-miner-1",
  
  "maxBatteryDrain": 5,
  "enableOnBattery": false,
  "enableOnCharging": true,
  "minBatteryLevel": 30,
  "maxTemperature": 40,
  
  "hashBatchSize": 100,
  "sleepBetweenBatches": 100,
  
  "lowPowerMode": true,
  "backgroundHashBatchSize": 10
}
```

### Key Settings

#### Battery Settings
- `enableOnBattery`: Allow mining when on battery (default: `false`)
- `enableOnCharging`: Allow mining when charging (default: `true`)
- `minBatteryLevel`: Minimum battery % to continue mining (default: 30%)
- `maxBatteryDrain`: Max battery drain % per hour (default: 5%)

#### Thermal Settings
- `maxTemperature`: Maximum device temperature in Celsius (default: 40°C)

#### Performance Settings
- `hashBatchSize`: Hashes per batch in foreground (default: 100)
  - **Much smaller than PC miners** which use 1024+
- `sleepBetweenBatches`: Milliseconds to sleep between batches (default: 100ms)
  - PC miners use 0ms (continuous)

#### Background Settings
- `lowPowerMode`: Enable low-power background mining (default: `true`)
- `backgroundHashBatchSize`: Hashes per batch when backgrounded (default: 10)

## Usage

### TypeScript/JavaScript Integration

```typescript
import {MobileMiningService} from './services/mining/MobileMiningService';
import {RPCClient} from './services/rpc/RPCClient';

// Create RPC client
const rpcClient = new RPCClient({
  url: 'https://node.drachma.org:8332',
  username: 'miner',
  password: 'password',
});

// Create mobile mining service
const miningService = new MobileMiningService(rpcClient, {
  enableOnCharging: true,
  enableOnBattery: false,
  minBatteryLevel: 30,
  maxTemperature: 40,
  hashBatchSize: 100,
  sleepBetweenBatches: 100,
});

// Start mining
await miningService.startMining();

// Get statistics
const stats = miningService.getStats();
console.log(`Hash rate: ${stats.hashRate} H/s`);
console.log(`Shares found: ${stats.sharesFound}`);
console.log(`Battery level: ${stats.batteryLevel}%`);
console.log(`Temperature: ${stats.temperature}°C`);

// Set background mode (when app goes to background)
miningService.setBackgroundMode(true);

// Stop mining
miningService.stopMining();
```

### Redux Integration

```typescript
import {useDispatch, useSelector} from 'react-redux';
import {setMiningEnabled, updateMiningStats} from './store/slices/miningSlice';

function MiningScreen() {
  const dispatch = useDispatch();
  const mining = useSelector((state: RootState) => state.mining);
  
  const toggleMining = () => {
    dispatch(setMiningEnabled(!mining.isEnabled));
  };
  
  return (
    <View>
      <Text>Hash Rate: {mining.stats.hashRate} H/s</Text>
      <Text>Battery: {mining.stats.batteryLevel}%</Text>
      <Text>Temperature: {mining.stats.temperature}°C</Text>
      <Button title={mining.isEnabled ? 'Stop' : 'Start'} onPress={toggleMining} />
    </View>
  );
}
```

## Performance Expectations

Mobile mining has **much lower hash rates** than PC mining due to:
- Battery and thermal constraints
- Smaller hash batches
- Sleep intervals between batches
- Single-threaded operation
- ARM processor limitations

### Expected Hash Rates
- **Foreground mining**: 50-200 H/s
- **Background mining**: 5-20 H/s
- **PC CPU miner**: 1,000-10,000 H/s
- **PC GPU miner**: 100,000-1,000,000+ H/s

Mobile mining is primarily for:
- Supporting the network decentralization
- Educational purposes
- Testing the mobile app
- Contributing small amounts when device is charging

**Do not expect significant mining rewards from mobile mining.**

## Security Considerations

### Battery Safety
- Mining automatically stops on low battery
- Temperature monitoring prevents overheating
- Configurable limits for safe operation

### Network Security
- Use trusted pool URLs only
- Verify pool certificates
- Do not mine on untrusted networks

### Privacy
- Mobile miner does not share device information
- Worker name is user-configurable
- No tracking or analytics

## Troubleshooting

### Mining won't start
- Check battery level is above `minBatteryLevel`
- Ensure device is charging if `enableOnBattery` is `false`
- Verify pool URL is correct and reachable

### Mining stops automatically
- Battery level dropped below threshold
- Device temperature exceeded `maxTemperature`
- User unplugged device when `enableOnBattery` is `false`

### Low hash rate
- This is expected for mobile mining
- Reduce `sleepBetweenBatches` for higher rate (but more heat/battery drain)
- Increase `hashBatchSize` for higher rate (but more heat/battery drain)
- Keep device cool for sustained mining

### Device overheating
- Lower `maxTemperature` threshold
- Reduce `hashBatchSize`
- Increase `sleepBetweenBatches`
- Use only when device is charging and cool

## Technical Architecture

### Implementation Details
- **Language**: TypeScript (React Native)
- **Hashing**: SHA256d (mobile-optimized, no AVX2)
- **Threading**: Single-threaded or minimal threading
- **Protocol**: Stratum V1 (lightweight)
- **Storage**: Redux state management

### Code Structure
```
mobile-client/src/
├── services/
│   └── mining/
│       └── MobileMiningService.ts    # Main mining service
├── store/
│   └── slices/
│       └── miningSlice.ts            # Redux state management
└── mobile_miner_config.example.json  # Configuration example
```

### Differences from PC Miners
The mobile miner is in `/mobile-client/src/services/mining/` and is **completely separate** from:
- `/miners/cpu/` - Multi-threaded CPU miner with AVX2
- `/miners/gpu-cuda/` - CUDA GPU miner
- `/miners/gpu-opencl/` - OpenCL GPU miner

**Do not mix or reuse code between mobile and PC miners.**

## License

Same as the parent PARTHENON CHAIN project (MIT).
