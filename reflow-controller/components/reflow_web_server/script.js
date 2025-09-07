// Reflow Controller Dashboard JavaScript
class ReflowDashboard {
    constructor() {
        this.temperatureChart = null;
        this.updateInterval = 1000;
        this.init();
    }
    
    init() {
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', () => this.setup());
        } else {
            this.setup();
        }
    }
    
    setup() {
        this.initializeChart();
        this.startDataUpdates();
    }
    
    initializeChart() {
        this.temperatureChart = Highcharts.chart('temperatureChart', {
            chart: {
                type: 'spline',
                animation: false,
                backgroundColor: 'transparent'
            },
            title: { text: null },
            xAxis: {
                type: 'datetime',
                title: { text: 'Time' }
            },
            yAxis: {
                title: { text: 'Temperature (&deg;C)' },
                plotLines: [{
                    value: 0,
                    width: 1,
                    color: '#808080'
                }]
            },
            tooltip: {
                formatter: function() {
                    return '<b>Temperature</b><br/>' +
                           Highcharts.dateFormat('%H:%M:%S', this.x) + '<br/>' +
                           '<span style="color: #e74c3c; font-weight: bold;">' +
                           this.y.toFixed(1) + ' &deg;C</span>';
                }
            },
            legend: { enabled: false },
            plotOptions: {
                spline: {
                    marker: { enabled: false },
                    lineWidth: 3,
                    color: '#e74c3c'
                }
            },
            series: [{
                name: 'Temperature',
                data: []
            }],
            credits: { enabled: false }
        });
    }
    
    updateTemperatureData() {
        fetch('/temperature_data')
            .then(response => response.json())
            .then(data => {
                if (data && data.length > 0) {
                    this.temperatureChart.series[0].setData(data, true);
                    
                    const latest = data[data.length - 1];
                    if (latest) {
                        document.getElementById('currentTemp').innerHTML = latest[1].toFixed(1) + ' &deg;C';
                    }
                }
            })
            .catch(error => {
                console.error('Error fetching temperature data:', error);
            });
    }
    
    startDataUpdates() {
        this.updateTemperatureData();
        setInterval(() => this.updateTemperatureData(), this.updateInterval);
    }
}

// Initialize dashboard
new ReflowDashboard();

// Switch control function
function toggleReflowSwitch() {
    const btn = document.getElementById('reflowSwitchBtn');
    const stateSpan = document.getElementById('switchState');
    const currentState = btn.classList.contains('on');
    const newState = !currentState;
    
    fetch('/switch_control', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ state: newState })
    })
    .then(response => response.json())
    .then(data => {
        if (data.state) {
            btn.classList.add('on');
            stateSpan.textContent = 'ON';
        } else {
            btn.classList.remove('on');
            stateSpan.textContent = 'OFF';
        }
    })
    .catch(error => {
        console.error('Error controlling switch:', error);
    });
}
